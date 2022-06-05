#pragma once

#include <array>
#include <initializer_list>
#include <string_view>
#include <vulkan/vulkan_core.h>

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


    template <class T>
    bool obeysInequality(T lhs, T rhs, vkRequirements::reqOperator op)
    {
        switch(op)
        {
        case vkRequirements::reqOperator::eq: return lhs == rhs;
        case vkRequirements::reqOperator::ne: return lhs != rhs;
        case vkRequirements::reqOperator::lt: return lhs < rhs;
        case vkRequirements::reqOperator::gt: return lhs > rhs;
        case vkRequirements::reqOperator::le: return lhs <= rhs;
        case vkRequirements::reqOperator::ge: return lhs >= rhs;
        default:
            throw Ex(fmt::format("Invalid operator '{}' for criteria '{}'", op, lhs));
        }
    }
}
