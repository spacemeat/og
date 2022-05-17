#include <fmt/format.h>
#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"

namespace og
{
    Engine::Engine(std::string_view configPath)
    {
        auto tr = hu::Trove::fromFile(configPath, {hu::Encoding::utf8}, hu::ErrorResponse::mum);
        if (auto t = std::get_if<hu::Trove>(& tr))
        {
            config = og::engine::engineConfig { t->root() };
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

    void Engine::init(std::string const & appName, std::array<int, 3> const & appVersion)
    {
        log("engine init");

        initVkInstance(appName, appVersion);
        initVkDevices();
        initWindowEnvironment();
        initViews(appName);
    }

    void Engine::initViews(std::string const & appName)
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
