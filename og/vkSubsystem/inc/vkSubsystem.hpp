#include "deviceCreator.hpp"

namespace og
{
    class VkQueueFamilySubsystem
    {
    public:
        VkQueueFamilySubsystem();
        ~VkQueueFamilySubsystem();

        void create(og::QueueFamilySubsystem & pack);

    private:
        void destroy();

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
    public:
        VkDeviceSubsystem(AbilityResolver & abilityResolver);
        ~VkDeviceSubsystem();

        void create(og::DeviceSubsystem & pack);

        VkQueueFamilySubsystem & getQueueFamily(std::size_t queueFamilyIdx) { return queueFamilies[queueFamilyIdx]; }

    private:
        void destroy();

        int physicalDeviceIdx;
        std::vector<VkQueueFamilySubsystem> queueFamilies;
        VkFeatures features;
        VkProperties properties;
        std::unordered_map<std::string_view, int> abilityCache;
        VkPhysicalDevice physicalDevice;
        AbilityResolver & abilityResolver;  // learn from parent
    };

    class VkDeviceGroup
    {
    public:
        VkDeviceGroup();
        ~VkDeviceGroup();

        void create(og::DeviceSubsystem & pack);

        VkDeviceSubsystem & getDevice(std::size_t deviceIdx) { return devices[deviceIdx]; }

    private:
        void destroy();

        std::size_t idx;
        std::string_view name;

        std::vector<VkDeviceSubsystem> devices;
        AbilityResolver abilityResolver;
    };

    class VkEngineSubsystem
    {
    public:
        VkEngineSubsystem();
        ~VkEngineSubsystem();

        void create(og::DeviceSubsystem & pack);

        VkDeviceGroup & getDeviceGroup(std::string_view deviceGroupName) { return getDeviceGroup(deviceGroupIdxs[deviceGroupName]); }
        VkDeviceGroup & getDeviceGroup(std::size_t deviceGroupIdx) { return deviceGroups[deviceGroupIdx]; }

    private:
        void destroy();

        std::size_t engineIdx;
        std::string_view name;

        std::unordered_map<std::string_view, std::size_t> deviceGroupIdxs;
        std::vector<VkDeviceGroup> deviceGroups;
    };

    class VkSubsystem
    {
    public:
        VkSubsystem(std::string const & configPath, ProviderAliasResolver & aliases,
                    AbilityCollection & abilities, std::string_view appName_c, version_t appVersion_c);
        ~VkSubsystem();

        void create(
            std::vector<std::tuple<std::string_view, std::string_view, size_t>> const & schedule,
            std::vector<char const *> const & requiredExtensions,
            std::vector<char const *> const & requiredLayers);
        
        // recreateEngine()

        VkEngineSubsystem & getEngine(std::string_view engineName) { return getEngine(engineIdxs[engineName]); }
        VkEngineSubsystem & getEngine(std::size_t engineidx) { return engines[engineidx]; }
        
        void destroy();

    private:
        // something like this
        void destroyAllEngines();
        void destroyDebugMessengers();
        void destroyVkInstance();

        ProviderAliasResolver const & aliases;
        AbilityCollection const & abilities;

        std::string_view appName_c;
        version_t appVersion_c;

        vkSubsystem::deviceConfig config_c;
        bool isCreated = false;

        std::unordered_map<std::string_view, std::size_t> engineIdxs;
        std::vector<VkEngineSubsystem> engines;

        VkInstance instance;

        version_t vulkanVersion;
        std::vector<char const *> extensions;
        std::vector<char const *> layers;
        std::vector<std::tuple<std::string_view, abilities::debugUtilsMessenger_t>> debugMessengers;
        std::vector<VkValidationFeatureEnableEXT> enabledValidations;
        std::vector<VkValidationFeatureDisableEXT> disabledValidations;
        std::vector<VkDebugUtilsMessengerCreateInfoEXT> debugMessengerObjects;
        std::optional<VkValidationFeaturesEXT> validationFeatures;
    };
}
