#include "../inc/deviceCreator.hpp"
#include "../../engine/inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../inc/app.hpp"
#include "../../abilities/inc/abilityResolver.hpp"


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
    void * pUserData)
{
    auto logTags = og::enumToNum(og::logger::logTags::vulkan) |
                   og::enumToNum(og::logger::logTags::validation);
    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        { logTags |= og::enumToNum(og::logger::logTags::verbose); }
    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        { logTags |= og::enumToNum(og::logger::logTags::info); }
    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        { logTags |= og::enumToNum(og::logger::logTags::warn); }
    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        { logTags |= og::enumToNum(og::logger::logTags::error); }

    if (messageType & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        { logTags |= og::enumToNum(og::logger::logTags::general); }
    if (messageType & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        { logTags |= og::enumToNum(og::logger::logTags::validation); }
    if (messageType & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        { logTags |= og::enumToNum(og::logger::logTags::performance); }

    if (pCallbackData->messageIdNumber != 0)
    {
        og::log(og::numToEnum<og::logger::logTags>(logTags), fmt::format("Vk: {} #{}: {}",
            pCallbackData->pMessageIdName, pCallbackData->messageIdNumber,
            pCallbackData->pMessage));
    }
    else
    {
        og::log(og::numToEnum<og::logger::logTags>(logTags), fmt::format("Vk: {}: {}",
            pCallbackData->pMessageIdName, pCallbackData->pMessage));
    }
    // TODO: labels in queues and command buffers, and objects

    return VK_FALSE;
}


static VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo(
    og::abilities::debugUtilsMessenger_t const & cfg)
{
    return VkDebugUtilsMessengerCreateInfoEXT
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = static_cast<VkDebugUtilsMessageSeverityFlagsEXT>(cfg.get_severity()),
        .messageType = static_cast<VkDebugUtilsMessageTypeFlagsEXT>(cfg.get_type()),
        .pfnUserCallback = debugCallback
    };
}

namespace og
{
    using crit = abilities::universalCriteria;
    using critKinds = og::abilities::criteriaKinds;

    DeviceCreator::DeviceCreator(std::string_view configPath, ProviderAliasResolver & aliases, AbilityCollection & abilities,
                                 std::string_view appName_c, version_t appVersion_c)
    : aliases(aliases), abilities(abilities)
    {

    }

    // TODO: Document this process, because it is involved.

    /*
    Instance creation involves ability include and criteria specifically
    for the instance, followed by the specific include and criteria resolutions for each
    device profile group in order. We're just looking for extensions
    for now, as in the total superset of any we might want, depending
    on the device profile groups we use, each one's ability set, and the
    vulkan API version. Once we have such an instance, we can query for
    proper features and properties on each physical device, and determine
    the actual extensions, layers, etc. that we'll need. Later on we'll
    destroy this instance and rebuild it with the more minimal extension
    set, as well as the layers if we're debugging.
    */

    void DeviceCreator::gatherExploratoryInstanceExtensions()
    {
        // get vulkan runtime version
        uint32_t vulkanVersion = 0;
        VKR(vkEnumerateInstanceVersion(& vulkanVersion));
        availableVulkanVersion = version_t { vulkanVersion };
        if (availableVulkanVersion < config_c.get_minVulkanVersion())
            { Ex(fmt::format("Vulkan version supported ({}) does not meet the minimum specified ({}).",
                availableVulkanVersion, config_c.get_minVulkanVersion())); }
        utilizedVulkanVersion = availableVulkanVersion;
        if (availableVulkanVersion.bits > version_t { config_c.get_maxVulkanVersion() }.bits)
            { utilizedVulkanVersion = config_c.get_maxVulkanVersion(); }

        log(fmt::format("System supports Vulkan API version ({})", availableVulkanVersion));
        log(fmt::format("Using Vulkan API version ({})", utilizedVulkanVersion));

        // get available instance extensions
        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, & count, nullptr);
        availableInstanceExtensions.resize(count);
        vkEnumerateInstanceExtensionProperties(nullptr, & count, availableInstanceExtensions.data());
        for (int i = 0; i < count; ++i)
        {
            availableInstanceExtensionNames.insert(
                std::string_view {availableInstanceExtensions[i].extensionName});
        }

        // get instance extensions we may wish to use from each
        // criteria and recursively through abilities
        auto fn = [this](crit const & criteria_c, std::vector<std::string_view> & extensions)
        {
            bool ok = true;
            bool foundProfile = false;

            if (ok && criteria_c.get_vulkanVersion().has_value())
            {
                ok = checkVulkan(* criteria_c.get_vulkanVersion(), utilizedVulkanVersion);
                foundProfile = ok;
            }
            if (ok && criteria_c.get_extensions().size() > 0)
            {
                auto const & exts_c = criteria_c.get_extensions();
                for (auto const & ext_c : exts_c)
                {
                    ok = ok && checkExtension(ext_c, availableInstanceExtensionNames);
                    if (ok)
                        { extensions.push_back(ext_c); }
                }
            }

            return std::make_tuple (ok, foundProfile);
        };

        std::vector<std::string_view> extensions;
        Accumulator accum(extensions);

        { // nested scope keeping me careful about not reusing ar
            AbilityResolver ar { aliases, abilities };
            for (auto inc_c : config_c.get_instanceInclude())
                { ar.include(inc_c); }

            if (auto const & criteria_c = config_c.get_sharedInstanceCriteria();
                criteria_c.has_value())
            {
                ar.doCrit(criteria_c->get_name(), * criteria_c, false, fn, accum);
            }
        }

        for (auto const & profileGroup_c : config_c.get_deviceProfileGroups())
        {
            AbilityResolver ar { aliases, abilities };
            for (auto inc_c : profileGroup_c.get_include())
                { ar.include(inc_c); }

            ar.doProfleGroup(profileGroup_c.get_name(), profileGroup_c, false, fn, accum);
        }

        /*
            this.availableInstExtensions = VkGetAvailableExtensions()

            gar = ar
            for each instanceInclude inc:
                gar.include(inc)
            this.exploratoryExtensions += gar.gather(instExts, shaerdInstanceCriteria)

            for each device group dg:
                dar = ar
                for each dg.include inc:
                    dar.include(inc)
                this.exploratoryExtensions += dar.gather(instExts, dg.sharedCriteria, availableInstExtensions)
                for each dg.profile proCriteria:
                    par = dar
                    this.exploratoryExtensions += par.gather(instExts, proCriteria, availableInstExtensions)
        */
    }

    void DeviceCreator::makeExploratoryInstance()
    {
        /*
            this.exploratoryInstance = VkCreateInstance(..., this.exploratoryExtensions, (no layers), ...)
        */
    }

    void DeviceCreator::matchDeviceAbilities()
    {
        AbilityResolver ar(aliases, abilities);
        /*
            for each device group dg:
                dar = ar
                [dgFeatures, dgProperties, dgDevExts] = dar.gather(features | properties | devExts, dg.sharedCriteria, assumeAllAvailable)
                for each dg.profile proCriteria:
                    par = dar
                    proInteresting = & dg.interestingProviders[profileIndex]
                    proInteresting.[features, properties, devExts] = [dgFeatures, dgProperties, dgDevExts]
                    proInteresting.[features, properties, devExts] += par.gather(features | properties | devExts, proCriteria, assumeAllAvailable)

            for each physical device [physDevIdx, dev]:
                for each device group dg:
                    for each dg.interestingProviders [profileIdx, proInteresting]:
                        match dev against proInteresting
                        proUsed = dg.usedProviders[profileIdx]
                        if it matches:
                            dg.deviceProfiles[physDevIdx].bestProfileIdx = profileIdx
                            dg.deviceProfiles[physDevIdx].usedProviders = proUsed
                            break
        */
    }

    void DeviceCreator::scoreDevices()
    {

    }

    void DeviceCreator::gatherInstanceExtensionsAndLayers()
    {

    }

    void DeviceCreator::makeFinalInstance()
    {

    }

    void DeviceCreator::makeDevices()
    {

    }







    void DeviceCreator::initPhysVkDevices()
    {
        uint32_t vulkanVersion = 0;
        VKR(vkEnumerateInstanceVersion(& vulkanVersion));

        uint32_t physCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, & physCount, nullptr);
        std::vector<VkPhysicalDevice> phDevices(physCount);
        vkEnumeratePhysicalDevices(vkInstance, & physCount, phDevices.data());

        devices.resize(physCount);
        for (int physIdx = 0; physIdx < physCount; ++physIdx)
        {
            devices[physIdx].init(physIdx, phDevices[physIdx]);
        }

        auto const & profileGroups_c = config_c.get_deviceProfileGroups();
        deviceAssignments.resize(profileGroups_c.size());
    }

    void DeviceCreator::computeBestProfileGroupDevices(int groupIdx)
    {
        auto const & profileGroups_c = config_c.get_deviceProfileGroups();
        auto const & group_c = profileGroups_c[groupIdx];
        auto const & profiles_c = group_c.get_profiles();

        log(fmt::format(". Scoring device group {}:", profileGroups_c[groupIdx].get_name()));

        auto & deviceAssignmentGroup = deviceAssignments[groupIdx];
        if (deviceAssignmentGroup.hasBeenComputed)
            { return; }

        deviceAssignmentGroup.deviceSuitabilities.resize(devices.size());

        for (int devIdx = 0; devIdx < devices.size(); ++devIdx)
        {
            log(fmt::format(". . Scoring physical device {}:", devIdx));

            auto & device = devices[devIdx];
            auto & suitability = deviceAssignmentGroup.deviceSuitabilities[devIdx];
            suitability.physicalDeviceIdx = devIdx;
            suitability.profileCritera.resize(profiles_c.size());
            int profileIdx = device.findBestProfileIdx(groupIdx, profileGroups_c[groupIdx], suitability);
            suitability.bestProfileIdx = profileIdx;
            if (profileIdx >= 0)
            {
                std::vector<std::string_view> featureProviders;
                std::vector<std::string_view> propertyProviders;
                std::vector<std::string_view> queueFamilyPropertyProviders;
                auto getQueueFamilyProperties = [& featureProviders, & propertyProviders, & queueFamilyPropertyProviders](auto const & criteria)
                {
                    if (criteria.has_value())
                    {
                        auto const & features = criteria->get_features();
                        for (auto const & [key, _] : features)
                            { featureProviders.push_back(key); }
                        auto const & properties = criteria->get_properties();
                        for (auto const & [key, _] : properties)
                            { propertyProviders.push_back(key); }
                        auto const & queueFamilyProperties = criteria->get_queueFamilyProperties();
                        for (auto const & [key, _] : queueFamilyProperties)
                            { queueFamilyPropertyProviders.push_back(key); }
                    }
                };

                uint32_t devQueueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties2(device.physicalDevice, & devQueueFamilyCount, nullptr);
                std::vector<VkQueueFamilyProperties2> qfPropsVect(devQueueFamilyCount);
                suitability.queueFamilies.resize(devQueueFamilyCount);
                for (int i = 0; i < devQueueFamilyCount; ++i)
                {
                    suitability.queueFamilies[i].init(queueFamilyPropertyProviders);
                    qfPropsVect[i] = suitability.queueFamilies[i].mainStruct;
                }
                vkGetPhysicalDeviceQueueFamilyProperties2(device.physicalDevice, & devQueueFamilyCount, qfPropsVect.data());
                for (int i = 0; i < devQueueFamilyCount; ++i)
                {
                    suitability.queueFamilies[i].mainStruct = qfPropsVect[i];
                }

                auto && [queueFamilyGroupIdx, queueFamilyAlloc] =
                    device.findBestQueueFamilyAllocation(groupIdx, profileGroups_c[groupIdx], profileIdx);
                suitability.bestQueueFamilyGroupIdx = queueFamilyGroupIdx;
                if (queueFamilyGroupIdx >= 0)
                {
                    suitability.queueFamilyComposition = std::move(queueFamilyAlloc);
                }
            }
        }
        deviceAssignmentGroup.hasBeenComputed = true;
    }

    void DeviceCreator::assignDevices(int groupIdx, int numDevices)
    {
        auto const & profileGroups_c = config_c.get_deviceProfileGroups();
        auto const & group_c = profileGroups_c[groupIdx];
        auto & deviceAssignmentGroup = deviceAssignments[groupIdx];  // NOT the assignment index!

        for (int numAssignments = 0; numAssignments < numDevices; ++numAssignments)
        {
            int bestProfileIdx = group_c.get_profiles().size();
            int bestDeviceIdx = -1;

            for (int devIdx = 0; devIdx < devices.size(); ++devIdx)
            {
                // find the best device which is unassigned
                auto & device = devices[devIdx];
                if (device.isAssignedToDeviceProfileGroup)
                    { continue; }

                auto & suitability = deviceAssignmentGroup.deviceSuitabilities[devIdx];
                if (suitability.bestProfileIdx == -1 ||
                    suitability.bestQueueFamilyGroupIdx == -1)
                    { continue; }

                if (suitability.bestProfileIdx < bestProfileIdx)
                {
                    bestProfileIdx = suitability.bestProfileIdx;
                    bestDeviceIdx = devIdx;
                }
            }

            if (bestDeviceIdx == -1)
                { return; } // if any assignment fails, they'll fail every time, so bail now

            auto & device = devices[bestDeviceIdx];
            device.isAssignedToDeviceProfileGroup = true;
            device.groupIdx = groupIdx;
            device.profileIdx = bestProfileIdx;
            deviceAssignmentGroup.winningDeviceIdxs.push_back(bestDeviceIdx);
        }
    }

    void DeviceCreator::createAllVkDevices()
    {
        for (int devIdx = 0; devIdx < devices.size(); ++devIdx)
        {
            auto & device = devices[devIdx];
            device.createVkDevice();
        }
    }

    void DeviceCreator::destroyAllDevices()
    {
        for (int i = 0; i < devices.size(); ++i)
        {
            destroyDevice(i);
        }
    }

    void DeviceCreator::destroyDevice(int deviceIdx)
    {
        devices[deviceIdx].destroyVkDevice();
    }

}
