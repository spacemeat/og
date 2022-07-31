#include <fmt/format.h>
#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../inc/app.hpp"

namespace og
{
    void Engine::initAbilities()
    {
        auto const & appConfig = app->get_config();

        auto fn = [](std::string_view const & path, std::vector<hu::Trove> & troves, auto & umap){
            auto tr = hu::Trove::fromFile(path, {hu::Encoding::utf8}, hu::ErrorResponse::stderrAnsiColor);
            if (auto t = std::get_if<hu::Trove>(& tr))
            {
                troves.push_back(std::move(* t));
                auto const & map_c = t->root();
                for (int i = 0; i < map_c.numChildren(); ++i)
                {
                    auto const & chNode = map_c.child(i);
                    umap.emplace(chNode.key(), chNode);
                }
            }
            else
            {
                throw Ex(fmt::format("Could not load config at {}.", path));
            }
        };

        for (auto const & path : appConfig.get_providerAliasesPaths())
        {
            fn(path, abilitiesTroves, providerAliases);
        }
        for (auto const & path : appConfig.get_builtinAbilitiesPaths())
        {
            fn(path, abilitiesTroves, builtins);
        }
        for (auto const & path : appConfig.get_abilitiesPaths())
        {
            fn(path, abilitiesTroves, abilities);
        }
    }
}
