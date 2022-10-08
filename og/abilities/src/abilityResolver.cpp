#include "../inc/abilityResolver.hpp"
#include "../../app/inc/except.hpp"
#include "../../app/inc/utils.hpp"

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
        this->aliasResolver = & aliasResolver;
        this->abilityCollection = & abilityCollection;
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

    void AbilityResolver::invalidateAbilityProfileCache()
    {
        cache.clear();
    }

    abilities::abilityProfileGroup_t const * AbilityResolver::findAbility(std::string_view abilityName, bool caching)
    {
        // check for cached value
        if (auto cachedIt = cache.find(abilityName); cachedIt != cache.end())
        {
            auto group_p = cachedIt->second;
            return group_p;
        }
        else
        {
            for (auto const & libraryName : abilityIncludes)
            {
                auto const & group = abilityCollection->getLibrary(libraryName);
                auto const & abilities_c = group.get_abilities();
                auto ita_c = abilities_c.find(abilityName);
                if (ita_c != abilities_c.end())
                {
                    auto const & ability_c = ita_c->second;

                    if (caching)
                        { cache[abilityName] = & ability_c; }

                    return & ability_c;
                }
            }
        }

        return nullptr;
    }
}

