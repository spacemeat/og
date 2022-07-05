#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>

#include "../gen/inc/physicalVkDeviceProfileGroup.hpp"
#include "../../gen/inc/vkChainStructs.hpp"

namespace og
{
    struct QueueFamilyAlloc
    {
        uint32_t qfi;
        uint32_t count;
        VkDeviceQueueCreateFlags flags;
        std::vector<float> priorities;
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
        VkQueueFamilies queueFamilies;
        uint32_t bestProfileIdx;
        uint32_t bestQueueFamilyGroupIdx;
        QueueFamilyComposition queueFamilyComposition;
    };

    struct DevProfileGroupAssignment
    {
        int groupIdx = -1;
        bool hasBeenComputed = false;
        // one for each enumerated physical device
        std::vector<PhysicalDeviceSuitability> deviceSuitabilities;
        // winning physical device indices
        std::vector<int> winningDeviceIdxs;
    };

    class PhysVkDevice  // naming is hard
    {
    public:
        PhysVkDevice();
        void init(int physicalDeviceIdx, VkPhysicalDevice phdev);
        int findBestProfileIdx(int groupIdx, engine::physicalVkDeviceProfileGroup const & profileGroup, PhysicalDeviceSuitability & suitability);
        std::tuple<int, QueueFamilyComposition> findBestQueueFamilyAllocation(int groupIdx, engine::physicalVkDeviceProfileGroup const & group);

        bool checkDeviceExtension(std::string_view deviceExtension);
        bool checkQueueTypes(VkQueueFlagBits queueTypesIncluded, VkQueueFlagBits queueTypesExcluded);

        void createVkDevice();
        void destroyVkDevice();

        int physicalDeviceIdx;
        VkPhysicalDevice physicalDevice;

        std::vector<VkExtensionProperties> availableDeviceExtensions;

        std::map<std::string_view, int> featureProviderIndexMap;
        VkPhysicalDeviceFeatures2 availableDeviceFeatures;
        std::vector<std::tuple<VkStructureType, void *>> availableFeaturesIndexable;

        std::map<std::string_view, int> propertyProviderIndexMap;
        VkPhysicalDeviceProperties2 availableDeviceProperties;
        std::vector<std::tuple<VkStructureType, void *>> availablePropertiesIndexable;

        std::vector<VkQueueFamilyProperties2> availableQueueFamilies;

        bool isAssignedToDeviceProfileGroup = false;

        int groupIdx = -1;
        int profileIdx = -1;
        VkDevice device = nullptr;

        std::vector<std::string_view> utilizedDeviceExtensions;
        VkPhysicalDeviceFeatures2 utilizedDeviceFeatures;
        std::vector<std::tuple<VkStructureType, void *>> utilizedFeaturesIndexable;
        QueueFamilyComposition utilizedQueueFamilies;
    };

}
