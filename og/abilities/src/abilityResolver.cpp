#include "../inc/abilityResolver.hpp"
#include "../../engine/inc/except.hpp"
#include "../../engine/inc/utils.hpp"

namespace og
{
    using crit = abilities::universalCriteria;
    using critKinds = og::abilities::criteriaKinds;

    AbilityResolver::AbilityResolver(ProviderAliasResolver const & aliasResolver,
                                     AbilityCollection const & abilityCollection)
    : aliasResolver(aliasResolver),
      abilityCollection(abilityCollection)
    {
    }

    void AbilityResolver::include(std::string_view libraryName)
    {
        if (std::find(begin(abilityIncludes), end(abilityIncludes), libraryName)
            == end(abilityIncludes))
        {
            abilityIncludes.push_back(libraryName);
            auto const & group = abilityCollection.getLibrary(libraryName);
            for (auto const & include : group.get_include())
            {
                this->include(include);
            }
        }
    }

    void AbilityResolver::addAbility(std::string_view abilityName)
    {
        // check for cached value
        if (auto cachedIt = abilities.find(abilityName); cachedIt == abilities.end())
        {
            for (auto const & libraryName : abilityIncludes)
            {
                auto const & group = abilityCollection.getLibrary(libraryName);
                auto const & abilities_c = group.get_abilities();
                auto const & ability = abilities_c.at(abilityName);
                abilities[abilityName] = std::make_tuple(& ability, NotYetCached);

            }
        }
    }


    crit AbilityResolver::gatherInteresting(critKinds kinds)
    {

    }

    void AbilityResolver::invalidateAbilityCache()
    {

    }

    void AbilityResolver::invalidateBuiltinCache()
    {

    }
}

