#include "../inc/abilityResolver.hpp"
#include "../../engine/inc/except.hpp"
#include "../../engine/inc/utils.hpp"

namespace og
{
    using crit = vkRequirements::universalCriteria;
    using critKinds = og::vkRequirements::criteriaKinds;

    AbilityResolver::AbilityResolver(ProviderAliasResolver const & aliasResolver,
                                     BuiltinCollection const & builtinCollection,
                                     AbilityCollection const & abilityCollection)
    : aliasResolver(aliasResolver),
      builtinCollection(builtinCollection),
      abilityCollection(abilityCollection)
    {
    }

    void AbilityResolver::builtinInclude(std::string_view groupName)
    {
        if (std::find(begin(builtinIncludes), end(builtinIncludes), groupName)
            == end(builtinIncludes))
        {
            builtinIncludes.push_back(groupName);
            auto const & group = builtinCollection.getGroup(groupName);
            for (auto const & include : group.get_include())
            {
                builtinInclude(include);
            }
        }
    }

    void AbilityResolver::abilityInclude(std::string_view groupName)
    {
        if (std::find(begin(abilityIncludes), end(abilityIncludes), groupName)
            == end(abilityIncludes))
        {
            abilityIncludes.push_back(groupName);
            auto const & group = abilityCollection.getGroup(groupName);
            for (auto const & include : group.get_include())
            {
                abilityInclude(include);
            }
        }
    }

    void AbilityResolver::addCriteria(crit const & criteria)
    {
        for (auto const & ability : criteria.get_abilities())
        {
            // TODO HERE: Parse ability into profile query
            addAbility(ability);
        }
    }

    void AbilityResolver::addAbility(std::string_view abilityName)
    {
        // check for cached value
        if (auto cachedIt = abilities.find(abilityName); cachedIt == abilities.end())
        {
            for (auto const & groupName : abilityIncludes)
            {
                auto const & group = abilityCollection.getGroup(groupName);
                auto const & abilities_c = group.get_abilities();
                auto const & ability = abilities_c.at(abilityName);
                abilities[abilityName] = std::make_tuple(& ability, NotYetCached);
            }
        }
    }

    void AbilityResolver::addBuiltin(std::string_view builtinName)
    {
        // check for cached entry
        if (auto cachedIt = builtins.find(builtinName); cachedIt == builtins.end())
        {
            for (auto const & groupName : builtinIncludes)
            {
                auto const & group = builtinCollection.getGroup(groupName);
                auto const & builtins_c = group.get_builtins();
                auto const & builtin = builtins_c.at(builtinName);
                builtins[builtinName] = std::make_tuple(& builtin, NotYetCached);
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

