#include "../inc/engine.hpp"
#include <charconv>

namespace og
{
    version_t::version_t(std::string_view rhs)
    {
        char const * start = rhs.data();
        char const * end = rhs.data() + rhs.size();
        std::from_chars_result fcc;
        uint32_t major, minor, patch = 0;
        //std::errc ec;
        fcc = std::from_chars(start, end, major);
        if (fcc.ec != std::errc())
            { throw Ex(fmt::format("invalid version string '{}'", rhs)); }
        if (* fcc.ptr++ != '.')
            { throw Ex(fmt::format("invalid version string '{}'", rhs)); }
        fcc = std::from_chars(fcc.ptr, end, minor);
        if (fcc.ec != std::errc())
            { throw Ex(fmt::format("invalid version string '{}'", rhs)); }
        if (fcc.ptr < end && * fcc.ptr++ == '.')
        {
            fcc = std::from_chars(fcc.ptr, end, patch);
            if (fcc.ec != std::errc())
                { throw Ex(fmt::format("invalid version string '{}'", rhs)); }
        }
        bits = VK_MAKE_API_VERSION(0, major, minor, patch);
    }

    version_t::version_t(uint32_t vkVersion)
    : bits(vkVersion)
    {
    }

    version_t::version_t(char major, char minor, char patch)
    : bits(VK_MAKE_API_VERSION(0, major, minor, patch))
    {
    }

    bool operator ==(version_t lhs, version_t rhs)
    {
        return lhs.bits == rhs.bits;
    }

    bool operator !=(version_t lhs, version_t rhs)
    {
        return lhs.bits != rhs.bits;
    }

    bool operator <(version_t lhs, version_t rhs)
    {
        return lhs.bits < rhs.bits;
    }

    bool operator <=(version_t lhs, version_t rhs)
    {
        return lhs.bits <= rhs.bits;
    }

    bool operator >(version_t lhs, version_t rhs)
    {
        return lhs.bits < rhs.bits;
    }

    bool operator >=(version_t lhs, version_t rhs)
    {
        return lhs.bits <= rhs.bits;
    }
}
