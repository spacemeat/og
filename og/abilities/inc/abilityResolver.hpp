#pragma once

#include <vector>
#include <unordered_map>
#include "../gen/inc/vkChainStructs.hpp"
#include "../../vkRequirements/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/abilitiesGroups_t.hpp"
#include "../../abilities/gen/inc/providerAliases_t.hpp"
#include "../../abilities/gen/inc/builtinsGroups_t.hpp"
#include "providerAliasResolver.hpp"
#include "collections.hpp"
#include "vkPhysDevice.hpp"

namespace og
{
    class AbilityResolver
    {
        using crit = vkRequirements::universalCriteria;
        using critKinds = og::vkRequirements::criteriaKinds;

    public:
        AbilityResolver(ProviderAliasResolver const & aliasResolver,
                        BuiltinCollection const & builtinCollection,
                        AbilityCollection const & abilityCollection);

        void abilityInclude(std::string_view group);
        void builtinInclude(std::string_view group);
        void addCriteria(crit const & criteria);
    private:
        void addAbility(std::string_view ability);
        void addBuiltin(std::string_view builtin);

    public:
        template <typename VisitorFn>
        void forEachCriteria(VisitorFn fn)
        {
            //  for each ab in abilities:
            //      if ab ~= ? a op profile:
            //          abil = abilities[ab]
            //          if abil.profile op profile:
            //              forEachCriteria()
        }

        crit gatherInteresting(critKinds kinds);

        void invalidateAbilityCache();
        void invalidateBuiltinCache();

    private:
        ProviderAliasResolver const & aliasResolver;
        BuiltinCollection const & builtinCollection;
        AbilityCollection const & abilityCollection;
        std::vector<std::string_view> builtinIncludes;
        std::vector<std::string_view> abilityIncludes;

        const int NotYetCached = -2;
        const int NoGoodProfile = -1;
        std::unordered_map<std::string_view, std::tuple<og::abilities::builtin_t const *, int>> builtins;
        std::unordered_map<std::string_view, std::tuple<og::abilities::ability_t const *, int>> abilities;
    };




    /*
    struct ProviderGroup
    {
        std::vector<std::string_view> featureProviders;
        std::vector<std::string_view> propertyProviders;
        std::vector<std::string_view> queueFamilyPropertyProviders;
    };

    class BuiltinAbilities
    {
        bool initProviders(int vulkanVersion, ProviderGroup & providers);

        void computeProfiles(DeviceCapabilities & capabilities);

        bool hasAbility(std::string_view ability);
        std::string_view get_profile();
        bool is(og::vkRequirements::reqOperator op, std::string_view comparand);


    };
    */
}

/*



*/
