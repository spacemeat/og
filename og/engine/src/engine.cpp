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
            config = og::engine::engineConfig { configTrove.root() };
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

        initPhysVkDevices();

        std::vector<std::tuple<std::string_view, int>> deviceSchedule;

        for (auto const & work : appConfig.get_works())
        {
            for (auto const & [groupName, needed, wanted] : work.get_useDeviceProfileGroups())
            {
                deviceSchedule.emplace_back(groupName, needed);
            }
        }

        for (auto const & work : appConfig.get_works())
        {
            for (auto const & [groupName, needed, wanted] : work.get_useDeviceProfileGroups())
            {
                deviceSchedule.emplace_back(groupName, wanted - needed);
            }
        }

        for (auto const & [groupName, _] : deviceSchedule)
        {
            computeBestProfileGroupDevices(groupName);
        }

        for (auto const & [groupName, numDevices] : deviceSchedule)
        {
            assignDevices(groupName, numDevices);
        }

    }

    void Engine::shutdown()
    {
        if (vkInstance)
        {
            destroyAllVkDevices();
            destroyVkInstance();
        }
    }
}
