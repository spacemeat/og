#include <string>

#include "../inc/app.hpp"
#include "../../logger/inc/logger.hpp"
#include "../../vkDeviceCreator/inc/deviceCreator.hpp"

namespace og
{
    App::App(std::string const & configPath)
    : config_c { troves->loadAndKeep(configPath) },
      engine { config_c.get_engineConfigPath() }
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

        createVulkanSubsystem();


        engine.init();
    }

    void App::initAbilities()
    {
        providerAliases.loadAliases(config_c.get_providerAliasesPath());
        abilities.loadCollectionFiles(config_c.get_abilitiesPaths());
    }

    void App::initViews()
    {
        auto const & viewConfigs = config_c.get_views();
        views.resize(viewConfigs.size());
        for (int i = 0; i < viewConfigs.size(); ++i)
        {
            log(fmt::format("init view {}", i));
            auto & viewConfig = viewConfigs[i];
            if (std::holds_alternative<app::windowConfig_t>(viewConfig))
            {
                log(fmt::format("view is windowed"));
                initWindow(i, std::string { config_c.get_name() });
            }
            else
            {
                // initHmd(i, ...);
            }
        }
    }

    void App::createVulkanSubsystem()
    {
        DeviceCreator deviceCreator {
            config_c.get_deviceConfigPath(),
            providerAliases,
            abilities,
            config_c.get_name(),
            version_t { config_c.get_version() }
        };
        //create device schedule
        auto && sched = createSchedule();
        vk = deviceCreator.createVulkanSubsystem(sched);
    }

    std::vector<std::tuple<std::string_view, size_t>> const & App::createSchedule()
    {
        std::vector<std::tuple<std::string_view, size_t>> deviceSchedule;

        for (auto const & work : config_c.get_works())
        {
            for (auto const & [groupName, needed, wanted] : work.get_useDeviceProfileGroups())
            {
                deviceSchedule.emplace_back(groupName, needed);
            }
        }

        for (auto const & work : config_c.get_works())
        {
            for (auto const & [groupName, needed, wanted] : work.get_useDeviceProfileGroups())
            {
                deviceSchedule.emplace_back(groupName, wanted - needed);
            }
        }

        return deviceSchedule;
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
            if (std::holds_alternative<app::windowConfig_t>(config_c.get_views()[i]))
            {
                destroyWindow(i);
            }
        }
    }
}
