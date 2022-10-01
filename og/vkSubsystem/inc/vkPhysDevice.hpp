#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>

#include "../gen/inc/physDeviceProfileGroup.hpp"
#include "../../gen/inc/vkChainStructs.hpp"
//#include "deviceCreator.hpp"

namespace og
{
    class PhysVkDevice  // naming is hard
    {
       //DeviceCreator & deviceCreator;

    public:
        PhysVkDevice();
        void init(int physicalDeviceIdx, VkPhysicalDevice phdev);
        //int findBestProfileIdx(int groupIdx, vkSubsystem::physDeviceProfileGroup const & profileGroup, PhysicalDeviceSuitability & suitability);
        //std::tuple<int, QueueFamilyComposition> findBestQueueFamilyAllocation(int groupIdx, vkSubsystem::physDeviceProfileGroup const & group, int profileIdx);

        //bool checkDeviceExtension(std::string_view deviceExtension);
        //bool checkQueueTypes(VkQueueFlagBits queueTypesIncluded, VkQueueFlagBits queueTypesExcluded);

        //void createVkDevice();
        //void destroyVkDevice();

        int physicalDeviceIdx = -1;
        VkPhysicalDevice physicalDevice = nullptr;

        std::unordered_set<char const *> availableDeviceExtensions;

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
        //VkDevice device = nullptr;


        //DeviceCapabilities createdCapabilities;

        //std::vector<std::string_view> utilizedDeviceExtensions;
        //VkFeatures utilizedDeviceFeatures;
        //std::vector<std::tuple<VkStructureType, void *>> utilizedFeaturesIndexable;
        //std::vector<VkQueueFamilies> utilizedQueueFamilies;
        //QueueFamilyComposition utilizedQueueFamilyComposition;
    };


}
