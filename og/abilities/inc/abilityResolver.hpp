#pragma once

#include <vector>
#include <unordered_map>
#include "../gen/inc/vkChainStructs.hpp"
#include "../../abilities/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/abilityLibrary_t.hpp"
#include "../../abilities/gen/inc/providerAliases_t.hpp"
#include "providerAliasResolver.hpp"
#include "collections.hpp"
#include "vkPhysDevice.hpp"

namespace og
{
    template<class ...Args, std::size_t... I>
    auto toIntArray_impl(std::tuple<Args &...> const & collections, std::index_sequence<I...>)
    {
        return std::array<std::size_t, sizeof...(Args)> { std::get<I>(collections).size() ... };
    }

    template<class ...Args, typename Indices = std::make_index_sequence<sizeof...(Args)>>
    auto toIntArray(std::tuple<Args & ...> const & collections)
    {
        return toIntArray_impl(collections, Indices{});
    }

    template <class ...Args>
    struct AMark
    {
        AMark(std::tuple<Args & ...> const & args) : indices(toIntArray(args)) { }
        std::array<std::size_t, sizeof...(Args)> indices;
    };

    template<class ...Args, std::size_t... I>
    void resizer_impl(std::tuple<Args & ...> & collections, AMark<Args...> const & mark, std::index_sequence<I...>)
    {
        auto foo = { (std::get<I>(collections).resize(std::get<I>(mark.indices)), 0) ...};
    }

    template<class ...Args, typename Indices = std::make_index_sequence<sizeof...(Args)>>
    void resizer(std::tuple<Args & ...> & collections, AMark<Args...> const & mark)
    {
        resizer_impl(collections, mark, Indices{});
    }

    template <class ...Args>
    struct Accumulator
    {
        typedef AMark<Args...> Mark;
        Accumulator(Args &...args)
        : collections { args... }
        {
        }

        auto mark()
        {
            return Mark(collections);
        }

        void rollBack(Mark const & mark)
        {
            resizer(collections, mark);
        }

        std::tuple<Args &...> collections;
    };


    class AbilityResolver
    {
        using crit = abilities::universalCriteria;
        using critKinds = og::abilities::criteriaKinds;

    public:
        AbilityResolver(ProviderAliasResolver const & aliasResolver,
                        AbilityCollection const & abilityCollection);

        void include(std::string_view group);
        void addProfileGroup(crit const & criteria);
    private:
        void addAbility(std::string_view ability);
        void addBuiltin(std::string_view builtin);

    public:
        template <class ProfileGroup_t, typename VisitorFn, class Accumulator>
        int doProfleGroup(std::string_view profileGroupName, ProfileGroup_t const & profileGroup_c, bool builtinsOnly, VisitorFn && fn, Accumulator & accum)
        {
            int result = 0;
            auto const & criteria_c = profileGroup_c.get_sharedInstanceCriteria();
            auto mark = accum.mark();
            auto bool [ok, _] = doCrit(profileGroupName, criteria_c, builtinsOnly, std::forward<VisitorFn>(fn), accum);
            if (ok)
            {
                auto const & profiles_c = profileGroup_c.get_profiles();
                if (profiles_c.size() == 0)
                    { return 0; }   // return 0 for idx 0 or no profile (which is passing)

                for (auto const & profile_c : profiles_c)
                {
                    auto cmark = accum.mark();
                    auto bool [cok, foundProfile] = (doCrit(profile_c.get_name(), profile_c, builtinsOnly, std::forward<VisitorFn>(fn), accum));

                    if (cok == false)
                        { accum.rollBack(csize); }

                    if (foundProfile)
                    {
                        int idx = & profile_c - profiles_c.data();
                        return idx;
                    }
                }
            }

            // erase what we've done
            accum.rollBack(mark);

            return NoGoodProfile;
        }

        template <typename VisitorFn, class Accumulator>
        bool doCrit(std::string_view profileGroupName, crit const & criteria_c, bool builtinsOnly, VisitorFn && fn, Accumulator & accum)
        {
            bool ok = true;
            auto mark = accum.mark();

            //  ok = doEachAbility(profileGroupName, profileGroup.sharedCriteria, builtinsOnly, fn) != -1
            for (auto const & abilityName_c : criteria_c.get_abilities())
            {
                int idx = doAbility(abilityName_c, builtinsOnly, std::formward<VisitorFn>(fn), accum);
                cacheAbility(profileGroupName, idx);
                ok = idx != NoGoodProfile;
                if (! ok)
                    { break; }
            }

            //  ok = ok && fn(profileGroup.sharedCriteria)
            ok = ok && fn(criteria_c, accum);

            if (ok == false)
                { accum.rollBack(mark); }

            return ok;
        }

        template <class ProfileGroup_t, typename VisitorFn, class Accumulator>
        int doAbility(std::string_view myName, bool builtinsOnly, VisitorFn && fn, Accumulator & accum)
        {
            //      if ab ! ~= ? a op profile:
            //          op = ne; val = -1; ability = a
            //      else:
            //          op = op; val = profile; ability = ab
            //      abilityScore = get_ability(myName, ability, builtinsOnly, fn)
            //      if ! abilityscore op val:
            //          return -1
        }

        template <class ProfileGroup_t, typename VisitorFn>
        int get_ability(std::string_view myName, std::string_view abilityName, bool builtinsOnly, VisitorFn && fn)
        {
            /*
                if builtinsOnly == false &&
                   myName != abilityName:
                   if [ab, & pidx] = abilities[abilityName]:
                        if pidx == NotYetSet:
                            pidx = doProfleGroup(abilityName, ab, false, fn)
                        return pidx
                    else:
                        for each libname in abilityIncludes:
                            lib = abilityCollection.getGroup(libName)
                            if abObj = & lib.getAbility(abilityName):
                                pidx = doProfleGroup(abilityName, abObj, false, fn)
                                abilities[abilityName] = [abObj, pidx]
                                return pidx

                if [ab, & pidx] = builtins[abilityName]:
                    if pidx == NotYetSet:
                        pidx = doProfleGroup(abilityName, ab, true, fn)
                    return pidx
                else:
                    for each libname in builtinIncludes:
                        lib = builtinCollection.getGroup(libName)
                        if abObj = & lib.getAbility(abilityName):
                            pidx = doProfleGroup(abilityName, abObj, true, fn)
                            builtins[abilityName] = [abObj, pidx]
                            return pidx
            */
        }


        crit gatherInteresting(critKinds kinds);

        void invalidateAbilityCache();
        void invalidateBuiltinCache();

        int get_ability(std::string_view abilityName);

    private:
        ProviderAliasResolver const & aliasResolver;
        AbilityCollection const & abilityCollection;
        std::vector<std::string_view> builtinIncludes;
        std::vector<std::string_view> abilityIncludes;

        const int NotYetCached = -3;
        const int NoGoodProfile = -1;
        //std::unordered_map<std::string_view, std::tuple<og::abilities::abilityProfileGroup_t const *, int>> builtins;
        std::unordered_map<std::string_view, std::tuple<og::abilities::abilityProfileGroup_t const *, int>> abilities;
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
        bool is(og::abilitiesOperator op, std::string_view comparand);


    };
    */
}

/*



*/
