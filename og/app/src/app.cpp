#include <string>

#include "../inc/app.hpp"
#include "../../logger/inc/logger.hpp"
#include "../../vkDeviceCreator/inc/deviceCreator.hpp"

namespace og
{
    App::App(std::string const & configPath)
    : config { troves->loadAndKeep(configPath) },
      engine { config.get_engineConfigPath() }
    {
    }

    App::~App()
    {
    }

    void App::init()
    {
        log("app init");

        initAbilities();

        if (anyWindowViews())
        {
            initWindowEnvironment();

            if (anyVulkanWindowViews() && glfwSupportsVulkan() == false)
            {
                throw Ex("GLFW doesn't support vulkan. Unable to create view(s).");
            }
        }

        initViews();

        DeviceCreator deviceCreator {
            config.get_deviceConfigPath(),
            providerAliases,
            abilities,
            config.get_name(),
            version_t { config.get_version() }
        };
        vk = deviceCreator.createInstanceAndDevices();

        engine.init();
    }

    void App::initAbilities()
    {
        providerAliases.loadAliases(config.get_providerAliasesPath());
        abilities.loadCollectionFiles(config.get_abilitiesPaths());
    }

    void App::initViews()
    {
        auto const & viewConfigs = config.get_views();
        views.resize(viewConfigs.size());
        for (int i = 0; i < viewConfigs.size(); ++i)
        {
            log(fmt::format("init view {}", i));
            auto & viewConfig = viewConfigs[i];
            if (std::holds_alternative<app::windowConfig_t>(viewConfig))
            {
                log(fmt::format("view is windowed"));
                initWindow(i, std::string { config.get_name() });
            }
            else
            {
                // initHmd(i, ...);
            }
        }
    }

    void App::run()
    {
        // for each work, ...
        engine.enterLoop();
    }

    void App::shutdown()
    {
        engine.shutdown();

        // TODO: Turn this into VulkanSubsystem::~()
        if (vkInstance)
        {
            destroyAllVkDevices();
            destroyDebugMessengers();
            destroyVkInstance();
        }

        for (int i = 0; i < views.size(); ++i)
        {
            if (std::holds_alternative<app::windowConfig_t>(config.get_views()[i]))
            {
                destroyWindow(i);
            }
        }
    }
}
