#include "gen/inc/assetDb.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    auto tr = hu::Trove::fromFile("assets.hu");
    if (auto && t = std::get_if<hu::Trove>(& tr))
    {
        auto assetDb = og::assetDb::assetDb(t->root());
        auto assets = assetDb.get_assets();
        auto asset = assets[0];

        std::cout << "assetName: " << std::visit(
            [](auto && arg){ return arg.get_name(); },
             asset);

        std::cout << "cache: ";
        assetDb.syncWithSource();
    }
}
