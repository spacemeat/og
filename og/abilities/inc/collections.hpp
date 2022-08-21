#pragma once

#include <vector>
#include <unordered_map>
#include "../gen/inc/vkChainStructs.hpp"
#include "../../abilities/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/abilityLibrary_t.hpp"
#include "../inc/providerAliasResolver.hpp"
#include "vkPhysDevice.hpp"

namespace og
{
    class AbilityCollection
    {
        using crit = abilities::universalCriteria;
        using critKinds = abilities::criteriaKinds;

    public:

        void loadCollectionFiles(std::vector<std::string> const & paths)
        {
            for (auto const & path : paths)
            {
                loadCollectionFile(path);
            }
        }

        void loadCollectionFile(std::string const & path)
        {
            auto tr = hu::Trove::fromFile(path, {hu::Encoding::utf8}, hu::ErrorResponse::stderrAnsiColor);
            if (auto t = std::get_if<hu::Trove>(& tr))
            {
                troves.push_back(std::move(* t));
                auto bas_c = abilities::abilityCollection_t { troves.rbegin()->root() };
                for (auto const & [libName, lib] : bas_c.get_abilityLibraries())
                {
                    libraries_c[libName] = std::move(lib);
                }
            }
            else
            {
                throw Ex(fmt::format("Could not load library at {}.", path));
            }
        }

        abilities::abilityLibrary_t const & getLibrary(std::string_view libraryName) const
        {
            //return libraries_c[libraryName];
            return libraries_c.find(libraryName)->second;
        }

    private:
        std::vector<hu::Trove> troves;
        std::unordered_map<std::string_view, abilities::abilityLibrary_t> libraries_c;
    };
}
