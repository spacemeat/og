#include "../inc/troveKeeper.hpp"
#include "../inc/utils.hpp"
#include "../inc/except.hpp"

namespace og
{
    hu::Node TroveKeeper::loadAndKeep(std::string const & path)
    {
        auto tr = hu::Trove::fromFile(path, {hu::Encoding::utf8}, hu::ErrorResponse::stderrAnsiColor);
        if (auto t = std::get_if<hu::Trove>(& tr))
        {
            return keep(std::move(* t), path);
        }
        else
        {
            throw Ex(fmt::format("Could not load trove at {}: bad humon formatting.", path));
        }
    }

    hu::Node TroveKeeper::keep(hu::Trove && trove, std::string const & path)
    {
        if (! trove)
            { throw Ex(fmt::format("Could not keep nullish trove.", path)); }

        keptTroves[path] = std::move(trove);
        trove = {};
        return keptTroves[path].root();
    }

    hu::Trove & TroveKeeper::find(std::string const & path)
    {
        return keptTroves.at(path);
    }
}
