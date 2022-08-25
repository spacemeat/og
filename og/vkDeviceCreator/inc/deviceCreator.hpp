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
    struct InstanceInfo
    {

    };

    struct DeviceInfo
    {
        std::vector<char const *> extensions;
        std::vector<char const *> layers;
        std::vector<abilities::debugUtilsMessenger_t> debugMessengers;
        std::vector<VkValidationFeatureEnableEXT> enabledValidation;
        std::vector<VkValidationFeatureDisableEXT> disabledValidation;

        std::vector<std::string_view> deviceExtensions;
        std::vector<std::string_view> featureProviders;
        std::vector<std::string_view> propertyProviders;

        // built by makeDebugMessengersAndValidators()
        std::vector<VkDebugUtilsMessengerCreateInfoEXT> debugMessengerObjects;
        std::optional<VkValidationFeaturesEXT> validationFeatures;


        void consolidateCollections();
        void * makeDebugMessengersAndValidators();

        auto makeAccumulator()
        {
            return Accumulator { extensions, layers, debugMessengers,
                                 enabledValidation, disabledValidation,
                                 deviceExtensions, featureProviders, propertyProviders };
        }
    };

    struct QueueFamilyAlloc
    {
        uint32_t qfi;
        uint32_t count;
        VkDeviceQueueCreateFlags flags;
        std::vector<float> priorities;
        std::optional<VkQueueGlobalPriorityKHR> globalPriority;
    };

    struct QueueFamilyComposition
    {
        std::vector<QueueFamilyAlloc> queueFamilies;
    };

    struct ProfileSpecificCriteria
    {
        VkFeatures features;
        VkProperties properties;
    };

    struct PhysicalDeviceSuitability
    {
        uint32_t physicalDeviceIdx;
        // one for each profile
        std::vector<ProfileSpecificCriteria> profileCritera;
        // one for each queue family on the physical device
        // (once we've chosen the best profile idx)
        std::vector<VkQueueFamilies> queueFamilies;
        uint32_t bestProfileIdx;
        uint32_t bestQueueFamilyGroupIdx;
        QueueFamilyComposition queueFamilyComposition;
    };

    // One of these is stored per physical device group
    struct DevProfileGroupAssignment
    {
        int groupIdx = -1;
        bool hasBeenComputed = false;

        DeviceInfo expDeviceInfo;

        // one for each enumerated physical device
        std::vector<PhysicalDeviceSuitability> deviceSuitabilities;
        // winning physical device indices
        std::vector<int> winningDeviceIdxs;
    };

    struct DeviceCapabilities
    {
        int vulkanVersion;
        std::vector<std::string_view> extensions;
        std::vector<std::string_view> layers;

        std::vector<std::string_view> deviceExtensions;
        VkFeatures features;
        VkProperties properties;
        std::vector<VkQueueFamilies> queueFamilies;
        QueueFamilyComposition queueFamilyComposition;
    };

    class DeviceCreator
    {
    public:
        DeviceCreator(std::string_view configPath, ProviderAliasResolver & aliases, AbilityCollection & abilities,
                      std::string_view appName_c, version_t appVersion_c);

        // new upstart functiopns
        bool gatherExploratoryInstanceExtensions();
        bool requireGlfwExtensions();
        void consolidateExploratoryCollections();
        void makeExploratoryInstance();
        void * makeDebugMessengersAndValidators(InstanceInfo & instanceInfo);

        void initExploratoryPhysDevices();
        void matchDeviceAbilities();

        int getBestProfile();

        void scoreDevices();
        void gatherInstanceExtensionsAndLayers();
        void makeFinalInstance();
        void makeDevices();

        bool checkVulkan(std::string_view vulkanVersion, version_t available);
        bool checkExtension(std::string_view extension, std::unordered_set<char const *> const & available);
        bool checkLayer(std::string_view layer, std::unordered_set<char const *> const & available);

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
        void computeBestProfileGroupDevices(int groupIdx);
        void assignDevices(int groupIdx, int numDevices);
        void createAllVkDevices();

        void destroyAllDevices();
        void destroyDevice(int deviceIdx);


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
        std::unordered_set<char const *> availableInstanceExtensionNames;
        std::vector<VkLayerProperties> availableLayers;
        std::unordered_set<char const *> availableLayerNames;

        DeviceInfo expInstInfo;

        std::vector<std::string_view> utilizedExtensions;
        std::vector<std::string_view> utilizedLayers;

        VkInstance vkInstance;
        std::vector<VkDebugUtilsMessengerEXT> vkDebugMessengers;

        std::vector<PhysVkDevice> physDevices;

        // vector of [groupIdx, vector of [phIdx, profileIdx], winningPhysIdx]
        // matches 1-1 with groups
        std::vector<DevProfileGroupAssignment> deviceAssignments;

    };
}
