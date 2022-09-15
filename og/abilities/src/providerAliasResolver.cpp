#include "../inc/providerAliasResolver.hpp"
#include "../../app/inc/except.hpp"
#include "../../app/inc/utils.hpp"
#include "../../app/inc/troveKeeper.hpp"

namespace og
{
    using critKinds = og::abilities::criteriaKinds;

    void ProviderAliasResolver::loadAliases(std::string const & providerAliasesPath)
    {
        aliases_c = og::abilities::providerAliases_t { troves->loadAndKeep(providerAliasesPath) };
    }

    std::string_view ProviderAliasResolver::resolveAlias(std::string_view alias, version_t vulkanVersion, critKinds kind)
    {
        auto const & aliasesMap_c = aliases_c.get_providerAliases();
        if (auto it_c = aliasesMap_c.find(alias); it_c != aliasesMap_c.end())
        {
            auto const & aliasVector_c = it_c->second;
            for (auto const & alias_c : aliasVector_c)
            {
                auto version_c = version_t(alias_c.get_vulkanVersion());
                if (vulkanVersion >= version_c)
                {
                    switch(kind)
                    {
                    case critKinds::features:
                        if (auto pit = alias_c.get_features(); pit.has_value())
                            { return * pit; }
                    case critKinds::properties:
                        if (auto pit = alias_c.get_properties(); pit.has_value())
                            { return * pit; }
                    case critKinds::queueFamilyProperties:
                        if (auto pit = alias_c.get_queueFamilyProperties(); pit.has_value())
                            { return * pit; }
                    }
                }
            }
        }

        return alias;
    }
}
