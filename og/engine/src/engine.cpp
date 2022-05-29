#include <fmt/format.h>
#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"

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

    void Engine::init(std::string_view appName, version_t appVersion)
    {
        log("engine init");

        initVkInstance(appName, appVersion);

        if (anyWindowViews())
        {
            initWindowEnvironment();

            if (anyVulkanWindowViews() && glfwSupportsVulkan() == false)
            {
                Ex("GLFW doesn't support vulkan. Unable to create view(s).");
            }
        }

        initViews(appName);

        initVkPhysicalDevices();

    }

    void Engine::initViews(std::string_view appName)
    {
        auto const & viewConfigs = config.get_views();
        views.resize(viewConfigs.size());
        for (int i = 0; i < viewConfigs.size(); ++i)
        {
            log(fmt::format("init view {}", i));
            auto & viewConfig = viewConfigs[i];
            if (std::holds_alternative<engine::windowConfig_t>(viewConfig))
            {
                log(fmt::format("view is windowed"));
                initWindow(i, std::string { appName });
            }
            else
            {
                // initHmd(i, ...);
            }
        }
    }

    void Engine::shutdown()
    {
        for (int i = 0; i < views.size(); ++i)
        {
            if (std::holds_alternative<engine::windowConfig_t>(config.get_views()[i]))
            {
                destroyWindow(i);
            }
        }

        if (vkInstance)
            { destroyVkInstance(); }
    }
}
