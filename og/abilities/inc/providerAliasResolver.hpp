#pragma once

#include <vector>
#include <unordered_map>
#include "../../gen/inc/vkChainStructs.hpp"
#include "../../abilities/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/abilityLibrary_t.hpp"
#include "../../abilities/gen/inc/providerAliases_t.hpp"
#include "../../app/inc/utils.hpp"

namespace og
{
    class ProviderAliasResolver
    {
        using crit = abilities::universalCriteria;
        using critKinds = og::abilities::criteriaKinds;

    public:
        void loadAliases(std::string const & providerAliasesPath);
        std::string_view resolveAlias(std::string_view alias, version_t vulkanVersion, critKinds kind);

    private:
        hu::Trove trove;
        abilities::providerAliases_t aliases_c;
    };
}
