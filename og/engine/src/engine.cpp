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
            config.get_version();
        }
        else
        {
            throw Ex(fmt::format("Could not load engine config at {}.", configPath));
        }
    }

    Engine::~Engine()
    {
        for (int i = 0; i < views.size(); ++i)
        {
            if (std::holds_alternative<engine::windowConfig_t>(config.get_views()[i]))
            {
                destroyWindow(i);
            }
        }
    }

    void Engine::init(engine::appConfig const & appConfig)
    {
        log("engine init");

        initWindowEnvironment();

        auto const & viewConfigs = config.get_views();
        views.resize(viewConfigs.size());
        for (int i = 0; i < viewConfigs.size(); ++i)
        {
            log(fmt::format("init view {}", i));
            auto & viewConfig = viewConfigs[i];
            if (std::holds_alternative<engine::windowConfig_t>(viewConfig))
            {
                log(fmt::format("view is windowed"));
                initWindow(i, std::string { appConfig.get_name() });
            }
            else
            {
                // initHmd(i, ...);
            }
        }
    }
}
