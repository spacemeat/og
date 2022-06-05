#include "../logger/inc/logger.hpp"
#include "inc/engine.hpp"
#include "inc/app.hpp"
#include <fmt/format.h>

namespace og
{
    std::optional<App> app;
    std::optional<Logger> l;
    std::optional<Engine> e;

    struct makeGlobOpts
    {
        makeGlobOpts(std::string_view appConfigPath)
        {
            app.emplace(std::string {appConfigPath});
            l.emplace(std::string {app->get_config().get_loggerConfigPath()});
            e.emplace(std::string {app->get_config().get_engineConfigPath()});
        }

        ~makeGlobOpts()
        {
            e.reset();
            l.reset();
            app.reset();
        }
    };

    engine::appConfig getAppConfig(std::string_view appConfigPath, hu::Trove & trove)
    {
        auto tr = hu::Trove::fromFile(appConfigPath, {hu::Encoding::utf8}, hu::ErrorResponse::mum);
        if (auto && t = std::get_if<hu::Trove>(& tr))
        {
            trove = std::move(*t);
            return engine::appConfig { trove.root() };
        }
        else
        {
            throw Ex(fmt::format("Could not load app config at {}.", appConfigPath));
        }
    }
}


int main(int argc, char * argv[])
{
    using namespace og;

    try
    {
        auto globOpts = makeGlobOpts { "appConfig.hu" };

        app->init();
        app->run();

        log("Ended loop. Shutting down.\n");

        e->shutdown();
    }
    catch (Ex & ex)
    {
        if (l)
            { log(logger::logTags::error, ex.what(), ex.location()); }
        else
            { std::cout << ex.what() << "\n"; }
    }
    catch (std::exception & ex)
    {
        if (l)
            { log(logger::logTags::error, ex.what()); }
        else
            { std::cout << ex.what() << "\n"; }
    }
    catch (...)
    {
        std::cout << "house exception\n";
    }

    std::cout << "Donezo.\n";
}
