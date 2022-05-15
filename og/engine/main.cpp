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
        l.emplace(loggerConfigPath);
        e.emplace(engineConfigPath);
    }

    engine::appConfig getAppConfig(std::string_view appConfigPath)
    {
        auto tr = hu::Trove::fromFile(appConfigPath, {hu::Encoding::utf8}, hu::ErrorResponse::mum);
        if (auto && t = std::get_if<hu::Trove>(& tr))
        {
            return engine::appConfig { t->root() };
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
        auto && appConfig = getAppConfig("appConfig.hu");

        makeSingletons(appConfig.get_loggerConfigPath(),
                       appConfig.get_engineConfigPath());

        e->init(appConfig);
        e->enterLoop();

        std::cout << "Ended loop.\n";
    }
    catch (Ex & ex)
    {
        if (l)
            { logErr(ex.what(), ex.location()); }
        else
            { std::cout << ex.what() << "\n"; }
    }
    catch (std::exception & ex)
    {
        if (l)
            { logErr(ex.what()); }
        else
            { std::cout << ex.what() << "\n"; }
    }
    catch (...)
    {
        std::cout << "house exception\n";
    }

    std::cout << "Donezo.\n";
}
