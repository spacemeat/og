#include <fmt/format.h>
#include "../../engine/inc/except.hpp"
#include "../inc/AssetPack.hpp"

namespace og
{
    AssetPack::AssetPack(std::string_view packPath, int packIdx)
    : packIdx(packIdx)
    {
        auto tr = hu::Trove::fromFile(packPath);
        if (auto && t = std::get_if<hu::Trove>(& tr))
        {
            packData = og::assetDb::assetPack { t->root() };
        }
        else
        {
            throw Ex(fmt::format("Could not load AssetPack at {}.", packPath));
        }
    }
}