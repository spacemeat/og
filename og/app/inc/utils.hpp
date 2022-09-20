#pragma once

#include <array>
#include <initializer_list>
#include <string_view>
#include <vulkan/vulkan_core.h>
#include <fmt/format.h>

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

        char major() { return VK_API_VERSION_MAJOR(bits); }
        char minor() { return VK_API_VERSION_MINOR(bits); }
        char patch() { return VK_API_VERSION_PATCH(bits); }

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
    };
}

namespace og
{
    template <class T>
    auto enumToNum(T t) { return static_cast<std::underlying_type_t<T>>(t); }

    template <class T>
    auto numToEnum(std::underlying_type_t<T> n) { return static_cast<T>(n); }

    const int NotYetCached = -3;
    const int NoGoodProfile = -1;

    template<class T>
    void uniquifyVectorOfThings(std::vector<T> & things)
    {
        std::unordered_set<T> s;
        for (auto const & th : things)
            { s.insert(th); }

        things.assign(begin(s), end(s));
    }
}