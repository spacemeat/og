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
        version_t vulkanVersion;
        std::vector<char const *> extensions;
        std::vector<char const *> layers;
        std::vector<std::tuple<std::string_view, abilities::debugUtilsMessenger_t>> debugMessengers;
        std::vector<VkValidationFeatureEnableEXT> enabledValidations;
        std::vector<VkValidationFeatureDisableEXT> disabledValidations;

        void * makeDebugMessengersAndValidators();
        // built by makeDebugMessengersAndValidators()
        std::vector<VkDebugUtilsMessengerCreateInfoEXT> debugMessengerObjects;
        std::optional<VkValidationFeaturesEXT> validationFeatures;

        void consolidateCollections();
    };

    struct DeviceInfo
    {
        std::vector<char const *> deviceExtensions;
        std::vector<std::tuple<std::string_view, std::string_view>> features;
        std::vector<std::string_view> propertyProviders;

        void consolidateCollections();
        VkFeatures makeFeatures(VkFeatures const & templateFeatures);
    };

    struct InstanceDeviceInfo : public InstanceInfo, public DeviceInfo
    {
        void consolidateCollections()
        {
            static_cast<InstanceInfo *>(this)->consolidateCollections();
            static_cast<DeviceInfo *>(this)->consolidateCollections();
        }

        auto makeAccumulator()
        {
            return Accumulator { extensions, layers, debugMessengers,
                                 enabledValidations, disabledValidations,
                                 deviceExtensions, features, propertyProviders };
        }
    };

    // ----------------------------------------------

    struct QueueFamilySubsystem
    {
        uint32_t qfi;
        uint32_t count;
        std::vector<float> priorities;
        std::optional<VkQueueGlobalPriorityKHR> globalPriority;
        std::vector<VkQueue> createdQueues;
        VkQueueFamilyProperties queueFamilyProperties;
    };

    struct DeviceSubsystem
    {
        int physicalDeviceIdx;
        int deviceGroupIdx;
        std::string_view deviceGroupName;
        DeviceInfo info;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkFeatures features;
        VkProperties properties;
        std::vector<QueueFamilySubsystem> queueFamilies;
    };

    struct VulkanSubsystem
    {
        InstanceInfo info;
        VkInstance instance;
        std::vector<DeviceSubsystem> devices;
    };

    // ----------------------------------------------

    struct QueueFamilyAssignment
    {
        uint32_t qfi;
        uint32_t count;
        VkDeviceQueueCreateFlags flags;
        std::vector<float> priorities;
        std::optional<VkQueueGlobalPriorityKHR> globalPriority;
    };

    /*
    struct QueueFamilyComposition
    {
        std::vector<QueueFamilyAlloc> queueFamilies;
    };
    */

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
        InstanceDeviceInfo bestProfileDeviceInfo;
        // one for each queue family on the physical device
        // (once we've chosen the best profile idx)
        std::vector<VkQueueFamilies> queueFamilies;

        uint32_t bestProfileIdx;
        VkFeatures bestProfileFeatures;
        uint32_t bestQueueVillageProfile;
        std::vector<QueueFamilyAssignment> queueFamilyAssignments;
    };

    // One of these is stored per device group
    struct DevProfileGroupAssignment
    {
        int groupIdx = -1;
        bool hasBeenComputed = false;

        InstanceDeviceInfo expDeviceInfo;

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
        std::vector<QueueFamilyAssignment> queueFamilyComposition;
    };

    class DeviceCreator
    {
    public:
        DeviceCreator(std::string const & configPath, ProviderAliasResolver & aliases,
                      AbilityCollection & abilities, std::string_view appName_c, version_t appVersion_c);

        VulkanSubsystem createVulkanSubsystem(std::vector<std::tuple<std::string_view, size_t>> const & schedule);

        // new upstart functiopns
        bool gatherExploratoryInstanceExtensions();
        bool requireGlfwExtensions();
        void consolidateExploratoryCollections();
        void makeExploratoryInstance();
        //void * makeDebugMessengersAndValidators(InstanceInfo & instanceInfo);

        void initExploratoryPhysDevices();
        void matchDeviceAbilities(std::string_view deviceGroup);
        int getBestProfile(int devGroupIdx, int physDevIdx, VkFeatures const & features, VkProperties const & properties, int startingIdx);
        int getBestQueueVillageProfile(int devGroupIdx, int physDevIdx,
            VkFeatures const & availbleFeatures, VkProperties const & availableProperties,
            VkQueueFamilies const & availableQfProperties);

        void assignDevices(std::string_view deviceGroupName, int numDevices);

        //void gatherFinalCreationSet(std::string_view deviceGroupName, int numDevices, VulkanSubsystem & vs);
        void consolidateFinalCollections(VulkanSubsystem & vs);
        void makeFinalInstance(VulkanSubsystem & vs);
        void makeDevices(VulkanSubsystem & vs);
        void makeQueues(VulkanSubsystem & vs);

        int getDeviceGroupIdx(std::string_view deviceGroupName);
        bool checkVulkan(std::string_view vulkanVersion, version_t available);
        bool checkExtension(std::string_view extension, std::unordered_set<char const *> const & available);
        bool checkLayer(std::string_view layer, std::unordered_set<char const *> const & available);
        bool checkDeviceExtension(std::string_view extension, std::unordered_set<char const *> const & available);
        bool checkFeature(std::string_view provider_c, std::string_view feature_c, VkFeatures const & available);
        bool checkProperties(std::string_view provider_c, std::vector<std::tuple<std::string_view, og::abilities::op, std::string_view>> const & properties_c, VkProperties const & available);

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
        void createAllVkDevices();

        void destroyAllDevices();
        void destroyDevice(int deviceIdx);


    private:
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

        InstanceDeviceInfo expInstInfo;

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
