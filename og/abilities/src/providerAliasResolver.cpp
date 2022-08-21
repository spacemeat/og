#include "../inc/providerAliasResolver.hpp"
#include "../../engine/inc/except.hpp"
#include "../../engine/inc/utils.hpp"

namespace og
{
    using critKinds = og::abilities::criteriaKinds;

    void ProviderAliasResolver::loadAliases(std::string const & providerAliasesPath)
    {
        auto tr = hu::Trove::fromFile(providerAliasesPath, {hu::Encoding::utf8}, hu::ErrorResponse::stderrAnsiColor);
        if (auto t = std::get_if<hu::Trove>(& tr))
        {
            trove = std::move(* t);
            aliases_c = og::abilities::providerAliases_t { trove.root() };
        }
        else
        {
            throw Ex(fmt::format("Could not load provider aliases at {}.", providerAliasesPath));
        }
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
