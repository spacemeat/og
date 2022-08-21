#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>

#include "../gen/inc/physDeviceProfileGroup.hpp"
#include "../../gen/inc/vkChainStructs.hpp"
#include "../inc/deviceCreator.hpp"

namespace og
{
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

    class PhysVkDevice  // naming is hard
    {
       DeviceCreator & deviceCreator;

    public:
        PhysVkDevice(DeviceCreator & deviceCreator);
        void init(int physicalDeviceIdx, VkPhysicalDevice phdev);
        int findBestProfileIdx(int groupIdx, vkDeviceCreator::physDeviceProfileGroup const & profileGroup, PhysicalDeviceSuitability & suitability);
        std::tuple<int, QueueFamilyComposition> findBestQueueFamilyAllocation(int groupIdx, vkDeviceCreator::physDeviceProfileGroup const & group, int profileIdx);

        bool checkDeviceExtension(std::string_view deviceExtension);
        bool checkQueueTypes(VkQueueFlagBits queueTypesIncluded, VkQueueFlagBits queueTypesExcluded);

        void createVkDevice();
        void destroyVkDevice();

        int physicalDeviceIdx = -1;
        VkPhysicalDevice physicalDevice = nullptr;

        std::vector<VkExtensionProperties> availableDeviceExtensions;

        std::map<std::string_view, int> featureProviderIndexMap;
        VkPhysicalDeviceFeatures2 availableDeviceFeatures;
        std::vector<std::tuple<VkStructureType, void *>> availableFeaturesIndexable;

        std::map<std::string_view, int> propertyProviderIndexMap;
        VkPhysicalDeviceProperties2 availableDeviceProperties;
        std::vector<std::tuple<VkStructureType, void *>> availablePropertiesIndexable;

        // This is adjacent to the QF properties stored in a PhysicalDeviceSuitability.
        // That looks for chain structures; this only needs to consider queue types, so
        // we just get the basic structs.
        //std::vector<VkQueueFamilyProperties> availableQueueFamilies;
        VkQueueFlagBits availableQueueTypes;

        bool isAssignedToDeviceProfileGroup = false;

        int groupIdx = -1;
        int profileIdx = -1;
        VkDevice device = nullptr;


        DeviceCapabilities createdCapabilities;

        //std::vector<std::string_view> utilizedDeviceExtensions;
        //VkFeatures utilizedDeviceFeatures;
        //std::vector<std::tuple<VkStructureType, void *>> utilizedFeaturesIndexable;
        //std::vector<VkQueueFamilies> utilizedQueueFamilies;
        //QueueFamilyComposition utilizedQueueFamilyComposition;
    };


}
