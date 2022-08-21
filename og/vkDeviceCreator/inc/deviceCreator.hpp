#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "../gen/inc/vkChainStructs.hpp"
#include "../../abilities/gen/inc/universalCriteria.hpp"
#include "../../abilities/gen/inc/providerAliases_t.hpp"
#include "../../abilities/gen/inc/abilityLibrary_t.hpp"
#include "../../abilities/inc/providerAliasResolver.hpp"
#include "../../abilities/inc/abilityResolver.hpp"
#include "../../abilities/inc/collections.hpp"
#include "../gen/inc/deviceConfig.hpp"
#include "vkPhysDevice.hpp"

namespace og
{
    class DeviceCreator
    {
    public:
        DeviceCreator(std::string_view configPath, ProviderAliasResolver & aliases, AbilityCollection & abilities,
                      std::string_view appName_c, version_t appVersion_c);

        // new upstart functiopns
        void gatherExploratoryInstanceExtensions();
        void makeExploratoryInstance();
        void matchDeviceAbilities();
        void scoreDevices();
        void gatherInstanceExtensionsAndLayers();
        void makeFinalInstance();
        void makeDevices();

        // void initVkInstance();

    private:
        void destroyVkInstance();

    public:
        void createDebugMessengers(std::vector<VkDebugUtilsMessengerCreateInfoEXT> const & dbgMsgrs);
        void destroyDebugMessengers();

    public:
        // old confuesd functions
        std::vector<std::string_view> const & get_utilizedExtensions() { return utilizedExtensions; }
        std::vector<std::string_view> const & get_utilizedLayers() { return utilizedLayers; }

    private:
        // old confuesd functions
        void initPhysVkDevices();
        void computeBestProfileGroupDevices(int groupIdx);
        void assignDevices(int groupIdx, int numDevices);
        void createAllVkDevices();

        void destroyAllDevices();
        void destroyDevice(int deviceIdx);

    public:
        bool checkVulkan(std::string_view vulkanVersion);
        bool checkExtension(std::string_view extension);
        bool checkLayer(std::string_view layer);

    private:
        std::vector<hu::Trove> troves;

        ProviderAliasResolver const & aliases;
        AbilityCollection const & abilities;

        std::string_view appName_c;
        version_t appVersion_c;

        vkDeviceCreator::deviceConfig config_c;

        version_t availableVulkanVersion;
        version_t utilizedVulkanVersion;

        std::vector<VkExtensionProperties> availableInstanceExtensions;
        std::unordered_set<std::string_view> availableInstanceExtensionNames;
        std::vector<VkLayerProperties> availableLayers;

        std::vector<std::string_view> exploratoryExtensions;

        std::vector<std::string_view> utilizedExtensions;
        std::vector<std::string_view> utilizedLayers;

        VkInstance vkInstance;
        std::vector<VkDebugUtilsMessengerEXT> vkDebugMessengers;

        std::vector<PhysVkDevice> devices;

        // vector of [groupIdx, vector of [phIdx, profileIdx], winningPhysIdx]
        // matches 1-1 with groups
        std::vector<DevProfileGroupAssignment> deviceAssignments;

    };
}
