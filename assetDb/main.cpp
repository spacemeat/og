#include "gen-cpp/inc/og.hpp"
#include <fmt/format.h>

int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("assets.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
        auto assetDb = og::assetDb(t->root());
        auto assets = assetDb.get_assets();
        auto asset = assets[0];

        fmt::print("assetName: {}\n", asset.get_name());
        fmt::print("cache: ");
        assetDb.syncWithSource();
        fmt::print("\n");
    }
}
