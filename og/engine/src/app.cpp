#include "../inc/app.hpp"
#include "../../logger/inc/logger.hpp"

namespace og
{
    App::App(std::string_view configPath)
    {
        auto tr = hu::Trove::fromFile(configPath, {hu::Encoding::utf8}, hu::ErrorResponse::mum);
        if (auto t = std::get_if<hu::Trove>(& tr))
        {
            configTrove = std::move(* t);
            config = og::engine::appConfig { configTrove.root() };
        }
        else
        {
            throw Ex(fmt::format("Could not load app config at {}.", configPath));
        }
    }

    App::~App()
    {
    }

    void App::init()
    {
        log("app init");
        e->init();
    }

    void App::initViews()
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
        e->enterLoop();
    }

    void App::shutdown()
    {
        e->shutdown();

        for (int i = 0; i < views.size(); ++i)
        {
            if (std::holds_alternative<engine::windowConfig_t>(config.get_views()[i]))
            {
                destroyWindow(i);
            }
        }
    }
}
