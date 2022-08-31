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
    namespace internal
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

        // credit cppreference
        template<class ...Args, std::size_t... I>
        void rollerBacker_impl(std::tuple<Args & ...> & collections, AMark<Args...> const & mark, std::index_sequence<I...>)
        {
            [[maybe_unused]] auto foo = { (std::get<I>(collections).resize(std::get<I>(mark.indices)), 0) ...};
        }

        template<class ...Args, typename Indices = std::make_index_sequence<sizeof...(Args)>>
        void rollerBacker(std::tuple<Args & ...> & collections, AMark<Args...> const & mark)
        {
            rollerBacker_impl(collections, mark, Indices{});
        }

        template<class ...Args, std::size_t... I>
        void extender_impl(std::tuple<Args & ...> & collections, std::tuple<Args & ...> & rhs, std::index_sequence<I...>)
        {
            [[maybe_unused]] auto foo = { (std::get<I>(collections).extend(std::get<I>(rhs))) ...};
        }

        template<class ...Args, typename Indices = std::make_index_sequence<sizeof...(Args)>>
        void extender(std::tuple<Args & ...> & collections, std::tuple<Args & ...> & rhs)
        {
            extender_impl(collections, rhs, Indices{});
        }
    }

    template <class ...Args>    // TODO: concept for vector<Args...>
    struct Accumulator
    {
        typedef internal::AMark<Args...> Mark;

        Accumulator(Args &...args)
        : collections { args... }
        {
        }

        template <std::size_t N>
        auto & get()
        {
            return std::get<N>(collections);
        }

        auto mark()
        {
            return Mark(collections);
        }

        void rollBack(Mark const & mark)
        {
            internal::rollerBacker(collections, mark);
        }

        void extend(Accumulator<Args...> const & rhs)
        {
            internal::extender(collections, rhs);
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
        //void addProfileGroup(crit const & criteria);
    private:
        //void addAbility(std::string_view ability);
        //void addBuiltin(std::string_view builtin);

        abilities::abilityProfileGroup_t const * findAbility(std::string_view ability, bool caching = true);

    public:
        template <class ProfileGroup_t, typename VisitorFn, class Accumulator, class Payload>
        int doProfileGroup(std::string_view profileGroupName, ProfileGroup_t const & profileGroup_c, bool builtinsOnly, VisitorFn && fn, Accumulator & accum, Payload & payload, bool cacheAbilities = true)
        {
            int result = 0;
            auto const & criteria_c = profileGroup_c.get_sharedInstanceCriteria();
            auto mark = accum.mark();
            auto bool [ok, _] = doCrit(profileGroupName, criteria_c, builtinsOnly, std::forward<VisitorFn>(fn), accum, payload, cacheAbilities);
            if (ok)
            {
                auto const & profiles_c = profileGroup_c.get_profiles();
                if (profiles_c.size() == 0)
                    { return 0; }   // return 0 for idx 0 or no profile (which is passing)

                for (auto const & profile_c : profiles_c)
                {
                    auto cmark = accum.mark();
                    auto bool [cok, foundProfile] = (doCrit(profile_c.get_name(), profile_c, builtinsOnly, std::forward<VisitorFn>(fn), accum, payload, cacheAbilities));

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

        template <typename VisitorFn, class Accumulator, class Payload>
        bool doCrit(std::string_view profileGroupName, crit const & criteria_c, bool builtinsOnly, VisitorFn && fn, Accumulator & accum, Payload & payload, bool cacheAbilities = true)
        {
            bool ok = true;
            auto mark = accum.mark();

            //  ok = doEachAbility(profileGroupName, profileGroup.sharedCriteria, builtinsOnly, fn) != -1
            for (auto const & abilityName_c : criteria_c.get_abilities())
            {
                int idx = doAbility(abilityName_c, builtinsOnly, std::formward<VisitorFn>(fn), accum, payload, cacheAbilities);
                ok = idx != NoGoodProfile;
                if (! ok)
                    { break; }
            }

            //  ok = ok && fn(profileGroup.sharedCriteria)
            ok = ok && fn(criteria_c, accum, payload);

            if (ok == false)
                { accum.rollBack(mark); }

            return ok;
        }

        bool caching = true;

        template <class ProfileGroup_t, typename VisitorFn, class Accumulator, class Payload>
        int doAbility(std::string_view abilityName, bool builtinsOnly, VisitorFn && fn, Accumulator & accum, Payload & payload, bool cacheAbilities = true)
        {
            // TODO: interpret query syntax here: '? multiview >= 2+1'

            abilities::abilityProfileGroup_t const * pProfileGroup = nullptr;
            int cachedIdx = NotYetCached;

            if (auto it = abilities.find(abilityName);
                it == abilities.end())
            {
                // look up the ability in libraries
                pProfileGroup = findAbility(abilityName, cacheAbilities);
            }
            else
            {
                std::tie(pProfileGroup, cachedIdx) = * it;
            }

            if (pProfileGroup == nullptr)
                { return NoGoodProfile; }

            if (cachedIdx == NotYetCached)
            {
                // cachedIdx = doProfleGroup(...)
                cachedIdx = doProfileGroup(abilityName, * pProfileGroup, builtinsOnly,
                    std::forward<VisitorFn>(fn), accum, payload, cacheAbilities);
            }

            return cachedIdx;
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

        //std::unordered_map<std::string_view, std::tuple<og::abilities::abilityProfileGroup_t const *, int>> builtins;
        std::unordered_map<std::string_view,
                           std::tuple<abilities::abilityProfileGroup_t const *,
                                      int>> abilities;
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
