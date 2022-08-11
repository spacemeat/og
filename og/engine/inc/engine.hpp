#pragma once

#include <cassert>
#include <chrono>
#include <map>
#include <fmt/format.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../../gen/inc/og.hpp"
#include "../../abilities/gen/inc/abilities_t.hpp"
#include "../../abilities/inc/providerAliasResolver.hpp"
#include "except.hpp"
#include "utils.hpp"
#include "vkPhysDevice.hpp"

namespace og
{
    struct RequirementInfo
    {
        std::string_view name;
        std::array<std::tuple<vkRequirements::reqOperator, uint32_t>, 2> versionReqs;
        bool needsMet = false;
        uint32_t installedVersion = 0;
    };

    struct NeedInfo
    {
        std::string_view lhs;
        vkRequirements::reqOperator op;
        std::string_view rhs;
        bool needsMet = false;
    };


    class Engine
    {
    public:
        static constexpr std::array<int, 3> version = { 0, 0, 1 };

        using duration_t = std::chrono::high_resolution_clock::duration;
        using timePoint_t = std::chrono::high_resolution_clock::time_point;

        using engineTimerType = double;
        using engineTimeDuration = std::chrono::duration<engineTimerType, std::micro>;
        using engineTimePoint = std::chrono::time_point<std::chrono::high_resolution_clock, engineTimeDuration>;

        Engine(std::string configPath);
        ~Engine();

        engine::deviceConfig const & get_config() { return config; }
        engine::appConfig const & get_appConfig() { return appConfig; }

        void init();
        void shutdown();
    private:

    public:
        void enterLoop();

    private:
        bool iterateLoop();

        void initAbilities();

        void initVkInstance();
        void destroyVkInstance();

        void createDebugMessengers(std::vector<VkDebugUtilsMessengerCreateInfoEXT> const & dbgMsgrs);
        void destroyDebugMessengers();

    public:
        std::vector<std::string_view> const & get_utilizedExtensions() { return utilizedExtensions; }
        std::vector<std::string_view> const & get_utilizedLayers() { return utilizedLayers; }

    private:
        void initPhysVkDevices();
        void computeBestProfileGroupDevices(int groupIdx);
        void assignDevices(int groupIdx, int numDevices);
        void createAllVkDevices();

        void destroyAllVkDevices();
        void destroyVkDevice(int deviceIdx);

        void waitForIdleVkDevice();

    public:
        bool checkVulkan(std::string_view vulkanVersion);
        bool checkExtension(std::string_view extension);
        bool checkLayer(std::string_view layer);

    private:
        hu::Trove configTrove;
        engine::deviceConfig config;
        hu::Trove appConfigTrove;
        engine::appConfig appConfig;

        ProviderAliasResolver providerAliasResolver;

        std::vector<hu::Trove> abilitiesTroves;
        std::unordered_map<std::string_view, abilities::providerAlias> providerAliases;
        std::unordered_map<std::string_view, abilities::builtinAbility> builtins;
        std::unordered_map<std::string_view, abilities::ability> abilities;

        version_t availableVulkanVersion;
        version_t utilizedVulkanVersion;

        std::vector<VkExtensionProperties> availableExtensions;
        std::vector<VkLayerProperties> availableLayers;

        std::vector<std::string_view> utilizedExtensions;
        std::vector<std::string_view> utilizedLayers;

        VkInstance vkInstance;
        std::vector<VkDebugUtilsMessengerEXT> vkDebugMessengers;

        std::vector<PhysVkDevice> devices;

    public:

        // vector of [groupIdx, vector of [phIdx, profileIdx], winningPhysIdx]
        // matches 1-1 with groups
        std::vector<DevProfileGroupAssignment> deviceAssignments;
    private:
        /*
        // vector zips with physical devices
        std::vector<std::vector<VkExtensionProperties>> availableDeviceExtensions;
        // vector zips with physical devices
        //std::vector<std::map<std::string_view, void *>> availableDeviceFeatures;

        // zip
        std::vector<std::map<std::string_view, int>> featureProviderIndexMap;
        std::vector<VkPhysicalDeviceFeatures2> availableDeviceFeatures;
        std::vector<std::vector<std::tuple<VkStructureType, void *>>> availableFeaturesIndexable;
        std::vector<VkPhysicalDeviceFeatures2> utilizedDeviceFeatures;
        std::vector<std::vector<vstd::tuple<VkStructureType, void *>>> utilizedFeaturesIndexable;

        // zip
        std::vector<std::map<std::string_view, int>> propertyProviderIndexMap;
        std::vector<VkPhysicalDeviceFeatures2> availableDeviceProperties;
        std::vector<std::vector<std::tuple<VkStructureType, void *>>> availablePropertiesIndexable;

        // vector zips with physical devices
        //std::vector<std::map<std::string_view, void *>> availableDeviceProperties;
        // zips
        std::vector<std::vector<VkQueueFamilyProperties>> availableQueueFamilies;

        std::vector<VkDevice> devices;

        std::vector<VkExtensionProperties> utilizedExtensions;
        std::vector<VkLayerProperties> utilizedLayers;
        // vector zips with physical devices
        std::vector<std::vector<VkExtensionProperties>> utilizedDeviceExtensions;
        // vector zips with physical devices
        //std::vector<std::map<std::string_view, void *>> utilizedDeviceFeatures;
        // zips; for each device, a vector of [queueFamilyIndex, numQueues]
        std::vector<std::vector<std::tuple<uint32_t, uint32_t>>> utilizedQueueFamilies;

        // maps profileGroup name to [physical device index, profile index]
        std::map<std::string_view, std::tuple<int, int>> selectedPhysDeviceProfiles;
        */
    };

    extern std::optional<Engine> e;
}

