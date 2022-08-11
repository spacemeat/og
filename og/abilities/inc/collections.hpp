#pragma once

#include <vector>
#include <unordered_map>
#include "../gen/inc/vkChainStructs.hpp"
#include "../../vkRequirements/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/builtinsGroups_t.hpp"
#include "../inc/providerAliasResolver.hpp"
#include "vkPhysDevice.hpp"

namespace og
{
    template<class BaGroup_t, class BuiltinAbilities_t>
    class BuiltinAbilityCollection
    {
        using crit = vkRequirements::universalCriteria;
        using critKinds = vkRequirements::criteriaKinds;

    public:
        using baGroup_t = BaGroup_t;
        using bas_t = BuiltinAbilities_t;

        BuiltinAbilityCollection(std::vector<std::string> const & paths)
        {
            for (auto const & path : paths)
            {
                auto tr = hu::Trove::fromFile(path, {hu::Encoding::utf8}, hu::ErrorResponse::stderrAnsiColor);
                if (auto t = std::get_if<hu::Trove>(& tr))
                {
                    troves.push_back(std::move(* t));
                    auto bas_c = BuiltinCollection::baGroup_t { troves.rbegin()->root() };
                    groups_c.merge(std::move(bas_c.get_builtinsGroups()));
                }
                else
                {
                    throw Ex(fmt::format("Could not load collection at {}.", path));
                }
            }
        }

        BuiltinAbilities_t const & getGroup(std::string_view group) const { return groups_c[group]; }

    private:
        std::vector<hu::Trove> troves;
        std::unordered_map<std::string_view, BuiltinAbilities_t> groups_c;
    };

    typedef BuiltinAbilityCollection<abilities::builtinsGroups_t, abilities::builtins_t>        BuiltinCollection;
    typedef BuiltinAbilityCollection<abilities::abilitiesGroups_t, abilities::abilities_t>      AbilityCollection;
}
