#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>

#include "../gen/inc/physicalVkDeviceProfileGroup.hpp"

namespace og
{
    class PhysVkDevice  // naming is hard
    {
    public:
        PhysVkDevice();
        void init(int physicalDeviceIdx, VkPhysicalDevice phdev);
        int findBestProfileIdx(int groupIdx, engine::physicalVkDeviceProfileGroup const & profileGroup);

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

        std::vector<VkQueueFamilyProperties> availableQueueFamilies;

        // vector of [profileGroupName, profileIdx]
        std::vector<std::tuple<std::string_view, int>> profileGroupBestMatches;
        bool isAssignedToDeviceProfileGroup = false;

        std::string_view profileGroup;
        int profileIdx = -1;
        VkDevice device;

        std::vector<std::string_view> utilizedDeviceExtensions;
        VkPhysicalDeviceFeatures2 utilizedDeviceFeatures;
        std::vector<std::tuple<VkStructureType, void *>> utilizedFeaturesIndexable;
        std::vector<std::tuple<uint32_t, uint32_t, std::vector<float>>> utilizedQueueFamilies;
    };

}
