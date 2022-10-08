#include "../logger/inc/logger.hpp"
#include "inc/app.hpp"
#include <fmt/format.h>
#include "inc/troveKeeper.hpp"

namespace og
{
    //std::optional<App> app;
    std::optional<Logger> l;
    std::optional<TroveKeeper> troves;
    //std::optional<Engine> e;

    struct MakeGlobOpts
    {
        MakeGlobOpts(std::string const & loggerConfigPath)
        {
            //app.emplace(std::string {appConfigPath});
            l.emplace(loggerConfigPath);
            //e.emplace(std::string {app->get_config().get_deviceConfigPath()});
        }

        ~MakeGlobOpts()
        {
            //e.reset();
            l.reset();
            //app.reset();
        }
    };

    app::appConfig getAppConfig(std::string_view appConfigPath, hu::Trove & trove)
    {
        auto tr = hu::Trove::fromFile(appConfigPath, {hu::Encoding::utf8}, hu::ErrorResponse::mum);
        if (auto && t = std::get_if<hu::Trove>(& tr))
        {
            trove = std::move(*t);
            return app::appConfig { trove.root() };
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
        troves.emplace(TroveKeeper {});

        App app { "appConfig.hu" };

        MakeGlobOpts globs { app.get_config().get_loggerConfigPath() };

        app.init();
        app.run();

        log("Ended loop. Shutting down.\n");

        app.shutdown();
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

    troves.reset();
    std::cout << "Donezo.\n";
}
