#include "gen-cpp/inc/og.hpp"
#include <fmt/format.h>

int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("memman.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
    }
}
