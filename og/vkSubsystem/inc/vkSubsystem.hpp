#include "deviceCreator.hpp"

namespace og
{
    class VkQueueFamilySubsystem
    {
        uint32_t qfi;
        uint32_t count;
        std::vector<float> priorities;
        VkDeviceQueueCreateFlags flags;
        std::optional<VkQueueGlobalPriorityKHR> globalPriority;

        std::vector<VkQueue> createdQueues;
        VkQueueFamilies queueFamilyProperties;
    };

    class VkDeviceSubsystem
    {
        std::vector<VkQueueFamilySubsystem> queueFamilies;
        VkFeatures features;
        VkProperties properties;
        std::unordered_map<std::string_view, int> abilityCache;
    };

    class VkDeviceGroup
    {
        std::vector<VkDeviceSubsystem> devices;
        AbilityResolver abilityResolver;
    };

    class VkEngineSubsystem
    {
        std::unordered_map<std::string_view, VkDeviceGroup> deviceGroups;
    };

    class VkSubsystem
    {
        std::unordered_map<std::string_view, VkEngineSubsystem> engines;

        std::unordered_map<std::string_view, std::size_t> deviceGroupIdxs;
        VkInstance instance;

        version_t vulkanVersion;
        std::vector<char const *> extensions;
        std::vector<char const *> layers;
        std::vector<std::tuple<std::string_view, abilities::debugUtilsMessenger_t>> debugMessengers;
        std::vector<VkValidationFeatureEnableEXT> enabledValidations;
        std::vector<VkValidationFeatureDisableEXT> disabledValidations;

    };
}
