#pragma once

#include <vector>
#include <unordered_map>
#include "../../gen/inc/vkChainStructs.hpp"
#include "../../abilities/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/abilityLibrary_t.hpp"
#include "../../abilities/gen/inc/providerAliases_t.hpp"
#include "providerAliasResolver.hpp"
#include "collections.hpp"

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
            [[maybe_unused]] auto foo = 
            { (std::get<I>(collections).resize(std::get<I>(mark.indices)), 0) ...};
        }

        template<class ...Args, typename Indices = std::make_index_sequence<sizeof...(Args)>>
        void rollerBacker(std::tuple<Args & ...> & collections, AMark<Args...> const & mark)
        {
            rollerBacker_impl(collections, mark, Indices{});
        }

        template<class ...Args, std::size_t... I>
        void extender_impl(std::tuple<Args & ...> & collections, std::tuple<Args & ...> const & rhs, std::index_sequence<I...>)
        {
            [[maybe_unused]] auto foo = 
            { ((std::get<I>(collections).insert(end(std::get<I>(collections)), begin(std::get<I>(rhs)), end(std::get<I>(rhs)))), 0) ...};
        }

        template<class ...Args, typename Indices = std::make_index_sequence<sizeof...(Args)>>
        void extender(std::tuple<Args & ...> & collections, std::tuple<Args & ...> const & rhs)
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
            internal::extender(collections, rhs.collections);
        }

        std::tuple<Args &...> collections;
    };

    class AbilityResolver
    {
        using crit = abilities::universalCriteria;
        using critKinds = og::abilities::criteriaKinds;

    public:
        AbilityResolver();

        void init(ProviderAliasResolver const & aliasResolver,
                  AbilityCollection const & abilityCollection);

        void include(std::string_view group);

        template <class ProfileGroup_t, typename VisitorFn, class Accumulator, class Payload>
        int doProfileGroup(std::string_view profileGroupName, ProfileGroup_t const & profileGroup_c, VisitorFn && fn, Accumulator & accum, Payload & payload, bool cacheAbilities = true, int startingProfileIdx = 0)
        {
            int result = 0;
            bool ok = true;
            auto mark = accum.mark();
            auto const & criteria_c = profileGroup_c.get_sharedCriteria();
            if (criteria_c.has_value())
            {
                ok = doCrit(profileGroupName, * criteria_c, fn, accum, payload, cacheAbilities);
            }
            if (ok)
            {
                auto const & profiles_c = profileGroup_c.get_profiles();
                if (profiles_c.size() == 0)
                    { return 0; }   // return 0 for idx 0 or no profile (which is passing)

                for (int profileIdx_c = startingProfileIdx; profileIdx_c < profiles_c.size(); ++profileIdx_c)
                {
                    auto const & profile_c = profiles_c[profileIdx_c];
                    auto cmark = accum.mark();
                    auto cok = doCrit(profile_c.get_name(), profile_c, fn, accum, payload, cacheAbilities);
                    if (cok)
                        { return profileIdx_c; }
                    else
                        { accum.rollBack(cmark); }
                }
            }

            // erase what we've done
            accum.rollBack(mark);

            return NoGoodProfile;
        }

        template <typename VisitorFn, class Accumulator, class Payload>
        bool doCrit(std::string_view profileGroupName, crit const & criteria_c, VisitorFn && fn, Accumulator & accum, Payload & payload, bool cacheAbilities = true)
        {
            bool ok = true;
            auto mark = accum.mark();

            for (auto const & abilityName_c : criteria_c.get_abilities())
            {
                int idx = doAbility(abilityName_c, fn, accum, payload, cacheAbilities);
                ok = idx != NoGoodProfile;
                if (! ok)
                    { break; }
            }

            ok = ok && fn(criteria_c, accum, payload);

            if (ok == false)
                { accum.rollBack(mark); }

            return ok;
        }

        template <typename VisitorFn, class Accumulator, class Payload>
        int doAbility(std::string_view abilityName, VisitorFn && fn, Accumulator & accum, Payload & payload, bool cacheAbilities = true)
        {
            // TODO: interpret query syntax here: '? multiview >= 2+1'

            abilities::abilityProfileGroup_t const * pProfileGroup = nullptr;
            if (auto it = cache.find(abilityName);
                it == cache.end())
            {
                // look up the ability in libraries
                pProfileGroup = findAbility(abilityName, cacheAbilities);
            }
            else
            {
                pProfileGroup = it->second;
            }

            if (pProfileGroup == nullptr)
                { return NoGoodProfile; }

            return doProfileGroup(abilityName, * pProfileGroup,
                fn, accum, payload, cacheAbilities);
        }

        int getCachesize() const { return cache.size(); }
        void invalidateAbilityProfileCache();

    private:
        abilities::abilityProfileGroup_t const * findAbility(std::string_view ability, bool caching = true);
        ProviderAliasResolver const * aliasResolver;
        AbilityCollection const * abilityCollection;
        std::vector<std::string_view> abilityIncludes;

        std::unordered_map<std::string_view,
                           abilities::abilityProfileGroup_t const *> cache;
    };
}
