#include "../inc/abilityResolver.hpp"
#include "../../engine/inc/except.hpp"
#include "../../engine/inc/utils.hpp"

namespace og
{
    using crit = abilities::universalCriteria;
    using critKinds = og::abilities::criteriaKinds;

    AbilityResolver::AbilityResolver()
    {
    }

    void AbilityResolver::init(ProviderAliasResolver const & aliasResolver,
                               AbilityCollection const & abilityCollection)
    {
        aliasResolver = & aliasResolver;
        abilityCollection = & abilityCollection;
    }

    void AbilityResolver::include(std::string_view libraryName)
    {
        if (std::find(begin(abilityIncludes), end(abilityIncludes), libraryName)
            == end(abilityIncludes))
        {
            abilityIncludes.push_back(libraryName);
            auto const & group = abilityCollection->getLibrary(libraryName);
            for (auto const & include : group.get_include())
            {
                this->include(include);
            }
        }
    }

    abilities::abilityProfileGroup_t const * AbilityResolver::findAbility(std::string_view abilityName, bool caching)
    {
        // check for cached value
        if (auto cachedIt = abilities.find(abilityName); cachedIt == abilities.end())
        {
            for (auto & libraryName : abilityIncludes)
            {
                auto & group = abilityCollection->getLibrary(libraryName);
                auto & abilities_c = group.get_abilities();
                auto & ability = abilities_c.at(abilityName);

                if (caching)
                    { abilities[abilityName] = { & ability, NotYetCached }; }

                return & ability;
            }
        }

        return nullptr;
    }


    void AbilityResolver::invalidateAbilityCache()
    {

    }

    void AbilityResolver::invalidateBuiltinCache()
    {

    }
}

