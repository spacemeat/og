#pragma once

#include <exception>
#include <experimental/source_location>
#include <string_view>
#include <vulkan/vulkan.h>

namespace og
{
    using src_loc = std::experimental::source_location;

    class Ex : public std::runtime_error
    {
    public:
        Ex(std::string_view message,
           const src_loc & loc = src_loc::current())
        : std::runtime_error(message.data()), loc(loc) { }

        src_loc const & location() const { return loc; }

    private:
        src_loc loc;
    };

    static VkResult VKR(VkResult result, const src_loc & loc = src_loc::current())
    {
        if (static_cast<std::underlying_type_t<VkResult>>(result) < 0)
        {
            throw Ex(fmt::format("Vk error: {}", result), loc);
        }

        return result;
    }
}
