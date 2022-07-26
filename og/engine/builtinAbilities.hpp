#pragma once

#include <vector>
#include <map>
#include "../gen/inc/vkChainStructs.hpp"
#include "vkPhysDevice.hpp"

namespace og
{

    struct ProviderGroup
    {
        std::vector<std::string_view> featureProviders;
        std::vector<std::string_view> propertyProviders;
        std::vector<std::string_view> queueFamilyPropertyProviders;
    };

    class BuiltinAbilities
    {
        bool initProviders(int vulkanVersion, ProviderGroup & providers);

        void computeProfiles(DeviceCapabilities & capabilities);

        bool hasAbility(std::string_view ability);
        std::string_view get_profile();
        bool is(og::vkRequirements::reqOperator op, std::string_view comparand);


    };
}

/*



*/
