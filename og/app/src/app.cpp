#include <string>

#include "../inc/app.hpp"
#include "../../logger/inc/logger.hpp"
#include "../../vkSubsystem/inc/deviceCreator.hpp"

namespace og
{
    App::App(std::string const & configPath)
    : config_c { troves->loadAndKeep(configPath) }
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


        //engine.init();
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
        vk.emplace(
            config_c.get_deviceConfigPath(),
            providerAliases,
            abilities,
            config_c.get_name(),
            version_t { config_c.get_version() });

        //create device schedule
        auto && sched = createSchedule();
        std::vector<char const *> requiredExtensions;
        std::vector<char const *> requiredLayers;

        getVkExtensionsForGlfw(requiredExtensions);

        // TODO: get GLFW extensions here
        vk->create(sched, requiredExtensions, requiredLayers);
    }

    std::vector<std::tuple<std::string_view, std::string_view, size_t>> App::createSchedule()
    {
        std::vector<std::tuple<std::string_view, std::string_view, size_t>> deviceSchedule;

        for (auto const & engine_c : config_c.get_engines())
        {
            for (auto const & [groupName, needed, wanted] : engine_c.get_useDeviceProfileGroups())
            {
                deviceSchedule.emplace_back(engine_c.get_name(), groupName, needed);
            }
        }

        for (auto const & engine_c : config_c.get_engines())
        {
            for (auto const & [groupName, needed, wanted] : engine_c.get_useDeviceProfileGroups())
            {
                deviceSchedule.emplace_back(engine_c.get_name(), groupName, wanted - needed);
            }
        }

        return deviceSchedule;
    }

    void App::run()
    {
        // for each engine_c, ...
        //engine.enterLoop();
    }

    void App::shutdown()
    {
        // for each engine_c, ...
        //engine.shutdown();

        vk->destroy();

        for (int i = 0; i < views.size(); ++i)
        {
            if (std::holds_alternative<app::windowConfig_t>(config_c.get_views()[i]))
            {
                destroyWindow(i);
            }
        }
    }
}
