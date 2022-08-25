#include <fmt/format.h>
#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../inc/app.hpp"

namespace og
{
    Engine::Engine(std::string configPath)      // TODO: make this a string_view once humon gets a fix
    {
        auto tr = hu::Trove::fromFile(configPath, {hu::Encoding::utf8}, hu::ErrorResponse::mum);
        if (auto t = std::get_if<hu::Trove>(& tr))
        {
            configTrove = std::move(* t);
            config = og::engine::deviceConfig { configTrove.root() };
        }
        else
        {
            throw Ex(fmt::format("Could not load engine config at {}.", configPath));
        }
    }

    Engine::~Engine()
    {
        shutdown();
    }

    void Engine::init()
    {
        log("engine init");

        auto const & appConfig = app->get_config();

        initAbilities();

        initVkInstance();

        if (app->anyWindowViews())
        {
            app->initWindowEnvironment();

            if (app->anyVulkanWindowViews() && app->glfwSupportsVulkan() == false)
            {
                throw Ex("GLFW doesn't support vulkan. Unable to create view(s).");
            }
        }

        app->initViews();

        initPhysDevices();

        // vec of [groupIdx, numDevices]
        std::vector<std::tuple<int, int>> deviceSchedule;

        log("Rounding up device profile groups.");
        auto const & groups = config.get_vkDeviceProfileGroups();

        {
            //std::stringstream oss;
            //oss << HumonFormat(groups);
            //log(fmt::format("{}", oss.str()));
        }

        auto getGroup = [&](std::string_view groupName)
        {
            auto groupIt = find_if(begin(groups), end(groups),
                                   [& groupName](auto & a){ return a.get_name() == groupName; });
            if (groupIt == end(groups))
                { throw Ex(fmt::format("Invalid vkDeviceProfileGroups group name '{}'", groupName)); }

            return groupIt - begin(groups);
        };

        for (auto const & work : appConfig.get_works())
        {
            for (auto const & [groupName, needed, wanted] : work.get_useDeviceProfileGroups())
            {
                auto groupIdx = getGroup(groupName);
                deviceSchedule.emplace_back(groupIdx, needed);
            }
        }

        for (auto const & work : appConfig.get_works())
        {
            for (auto const & [groupName, needed, wanted] : work.get_useDeviceProfileGroups())
            {
                auto groupIdx = getGroup(groupName);
                deviceSchedule.emplace_back(groupIdx, wanted - needed);
            }
        }

        log("Scoring devices for profile groups.");
        for (auto const & [groupIdx, _] : deviceSchedule)
        {
            computeBestProfileGroupDevices(groupIdx);
        }

        log("Assigning winning devices to profile groups.");
        for (auto const & [groupIdx, numDevices] : deviceSchedule)
        {
            assignDevices(groupIdx, numDevices);
        }

        log("Creating devices.");
        createAllVkDevices();
    }

    void Engine::shutdown()
    {
        if (vkInstance)
        {
            destroyAllVkDevices();
            destroyDebugMessengers();
            destroyVkInstance();
        }
    }
}
