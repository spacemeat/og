#pragma once

#include <array>
#include <initializer_list>
#include <string_view>
#include <unordered_set>
#include <vulkan/vulkan_core.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
#include <humon/humon.hpp>
#include "../../gen/inc/og.hpp"

namespace og
{
    struct version_t
    {
        version_t() = default;
        version_t(version_t const & rhs) = default;
        version_t(version_t && rhs) = default;
        version_t(std::string_view rhs);
        version_t(uint32_t vkVersion);
        version_t(char major, char minor, char release);
        version_t & operator=(version_t const & rhs) = default;
        version_t & operator=(version_t && rhs) = default;

        char major() const { return VK_API_VERSION_MAJOR(bits); }
        char minor() const { return VK_API_VERSION_MINOR(bits); }
        char patch() const { return VK_API_VERSION_PATCH(bits); }

        uint32_t bits = 0;
    };

    bool operator ==(version_t lhs, version_t rhs);
    bool operator !=(version_t lhs, version_t rhs);
    bool operator <(version_t lhs, version_t rhs);
    bool operator <=(version_t lhs, version_t rhs);
    bool operator >(version_t lhs, version_t rhs);
    bool operator >=(version_t lhs, version_t rhs);
}

namespace hu
{
    template<>
    struct val<og::version_t>
    {
        static inline og::version_t extract(const Node & node)
        {
            return og::version_t(node.value().str());
        }

        static inline og::version_t extract(std::string_view valStr)
        {
            return og::version_t(valStr);
        }
    };
}

namespace og
{
    std::ostream & operator <<(std::ostream & out, const HumonFormat<og::version_t> & obj);
}

template <> struct fmt::formatter<og::HumonFormat<og::version_t>> : ostream_formatter {};

namespace og
{
    template <class T>
    auto enumToNum(T t) { return static_cast<std::underlying_type_t<T>>(t); }

    template <class T>
    auto numToEnum(std::underlying_type_t<T> n) { return static_cast<T>(n); }

    const int NotYetCached = -3;
    const int NoGoodProfile = -1;

    template <class T>
    std::vector<T> makeUnique(std::vector<T> const & inv)
    {
        std::unordered_set<T> added;
        std::vector<T> pared;

        for (auto val : inv)
        {
            if (auto [_, didInsert] = added.insert(val); didInsert)
                { pared.push_back(val); }
        }
        return pared;
    }

    template <class T, class U>
    std::vector<std::tuple<T, U>> makeUnique(std::vector<std::tuple<T, U>> const & inv)
    {
        std::unordered_set<T> added;
        std::vector<std::tuple<T, U>> pared;

        for (auto const & [name, val] : inv)
        {
            if (auto [_, didInsert] = added.insert(name); didInsert)
                { pared.emplace_back(name, val); }
        }
        return pared;
    }

    template <class T>
    bool containTheSame(std::vector<T> const & lhs, std::vector<T> const & rhs)
    {
        std::unordered_set<T> lhsu { begin(lhs), end(lhs) };
        std::unordered_set<T> rhsu { begin(rhs), end(rhs) };

        return false == std::any_of(begin(rhsu), end(rhsu), 
            [&lhsu](auto && rhse)
                { return lhsu.find(rhse) == end(lhsu); });
    }

    /*
    template <class TupleType, std::size_t I>
    std::vector<T> getElement(std::vector<T> const & rhs)
    {
        std::vector<T> res;
        std::transform(begin(rhs), end(rhs), 
            begin(res), [](auto elem) { return get<I>(elem); } );
        return res;
    }
    */

    template <std::size_t I, class T>
    auto getElement(std::vector<T> const & rhs)
    {
        using TT = std::tuple_element<0, T>::type;
        std::vector<TT> res;
        std::transform(begin(rhs), end(rhs), 
            begin(res), [](auto elem) { return std::get<I>(elem); } );
        return res;
    }
}
