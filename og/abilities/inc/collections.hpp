#pragma once

#include <vector>
#include <unordered_map>
#include <string_view>
#include "../gen/inc/vkChainStructs.hpp"
#include "../../abilities/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/abilityCollection_t.hpp"
#include "../inc/providerAliasResolver.hpp"
#include "../../app/inc/troveKeeper.hpp"
#include "../../gen/inc/enums.hpp"

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
            auto bas_c = abilities::abilityCollection_t { troves->loadAndKeep(path) };
            for (auto const & [libName, lib] : bas_c.get_abilityLibraries())
            {
                libraries_c[libName] = std::move(lib);
            }
        }

        abilities::abilityLibrary_t const & getLibrary(std::string_view libraryName) const
        {
            return libraries_c.find(libraryName)->second;
        }

    private:
        std::unordered_map<std::string_view, abilities::abilityLibrary_t> libraries_c;
    };
}
