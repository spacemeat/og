#include <fmt/format.h>
#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../inc/app.hpp"

namespace og
{
    void Engine::initAbilities()
    {
        auto const & appConfig = app->get_config();

        aliases.loadAliases(appConfig.get_providerAliasesPath());
        abilities.loadCollection(appConfig.get_abilitiesPaths());
    }
}
