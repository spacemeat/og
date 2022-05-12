#pragma once

#include <exception>
#include <experimental/source_location>
#include <string_view>

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
}
