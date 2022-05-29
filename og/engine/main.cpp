#include "../logger/inc/logger.hpp"
#include "inc/engine.hpp"
#include <fmt/format.h>

namespace og
{
    std::optional<Logger> l;
    std::optional<Engine> e;

    void makeSingletons(std::string_view loggerConfigPath,
                        std::string_view engineConfigPath)
    {
        l.emplace(std::string {loggerConfigPath});
        e.emplace(std::string {engineConfigPath});
    }

    void unmakeSingletons()
    {
        e.reset();
        l.reset();
    }

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
        hu::Trove appConfigTrove;
        auto const & appConfig = getAppConfig("appConfig.hu", appConfigTrove);

        makeSingletons(appConfig.get_loggerConfigPath(),
                       appConfig.get_engineConfigPath());

        e->init(std::string { appConfig.get_name() }, appConfig.get_version());
        e->enterLoop();

        std::cout << "Ended loop. Shutting down.\n";

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

    unmakeSingletons();
    std::cout << "Donezo.\n";
}
