#pragma once

#include <vector>
#include <unordered_map>
#include "../gen/inc/vkChainStructs.hpp"
#include "../../vkRequirements/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/abilitiesGroups_t.hpp"
#include "../../abilities/gen/inc/providerAliases_t.hpp"
#include "../../abilities/gen/inc/builtinsGroups_t.hpp"
#include "vkPhysDevice.hpp"
#include "../../engine/inc/utils.hpp"

namespace og
{
    class ProviderAliasResolver
    {
        using crit = vkRequirements::universalCriteria;
        using critKinds = og::vkRequirements::criteriaKinds;

    public:
        ProviderAliasResolver(std::string const & providerAliasesPath);
        std::string_view resolveAlias(std::string_view alias, version_t vulkanVersion, critKinds kind);

    private:
        hu::Trove trove;
        abilities::providerAliases_t aliases_c;
    };
}
