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

    void * InstanceInfo::makeDebugMessengersAndValidators()
    {
        void * createInfo_pNext = nullptr;

        debugMessengerObjects.resize(debugMessengers.size());
        for (int i = 0; i < debugMessengerObjects.size(); ++i)
        {
            auto const & cfg_c = debugMessengers[i];
            debugMessengerObjects[i] = std::move(makeDebugMessengerCreateInfo(cfg_c));
            debugMessengerObjects[i].pNext = nullptr;
            if (i > 0)
                { debugMessengerObjects[i - 1].pNext = & debugMessengerObjects[i]; }
        }
        if (debugMessengerObjects.size() > 0)
            { createInfo_pNext = & debugMessengerObjects[0]; }

        if (enabledValidation.size() > 0 || disabledValidation.size() > 0)
        {
            validationFeatures = VkValidationFeaturesEXT {
                .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
                .pNext = createInfo_pNext,
                .enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidation.size()),
                .pEnabledValidationFeatures = enabledValidation.data(),
                .disabledValidationFeatureCount = static_cast<uint32_t>(disabledValidation.size()),
                .pDisabledValidationFeatures = disabledValidation.data()
            };
            createInfo_pNext = & (* validationFeatures);
        }

        return createInfo_pNext;
    }

    void InstanceInfo::consolidateCollections()
    {
        uniquifyVectorOfThings(extensions);
        uniquifyVectorOfThings(layers);
        uniquifyVectorOfThings(enabledValidation);
        uniquifyVectorOfThings(disabledValidation);
    }


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

    bool DeviceCreator::gatherExploratoryInstanceExtensions()
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
                availableInstanceExtensions[i].extensionName);
        }

        /*  The rest of this function is about getting all the instance
            extensions that any ability in this device profile might want to use.
        */
        Accumulator accum(expInstInfo.instanceInfo.extensions, expInstInfo.instanceInfo.layers, expInstInfo.instanceInfo.debugMessengers,
                          expInstInfo.instanceInfo.enabledValidation, expInstInfo.instanceInfo.disabledValidation,
                          expInstInfo.deviceExtensions, expInstInfo.featureProviders, expInstInfo.propertyProviders);

        auto fn = [this](crit const & criteria_c, decltype(accum) & accum)
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
                        { accum.get<0>().push_back(ext_c.data()); }
                }
            }
            if (ok && criteria_c.get_layers().size() > 0)
            {
                auto const & lays_c = criteria_c.get_extensions();
                for (auto const & lay_c : lays_c)
                {
                    ok = ok && checkLayer(lay_c, availableLayerNames);
                    if (ok)
                        { accum.get<1>().push_back(lay_c.data()); }
                }
            }

            if (ok)
            {
                if (criteria_c.get_debugUtilsMessengers().size() > 0)
                {
                    auto const & dums = criteria_c.get_debugUtilsMessengers();
                    for (auto const & dum : dums)
                    {
                        accum.get<2>().push_back(dum);
                    }
                }

                if (criteria_c.get_validationFeatures().has_value())
                {
                    auto const & valFeats = * criteria_c.get_validationFeatures();
                    for (auto const & evf : valFeats.get_enabled())
                    {
                        accum.get<3>().push_back(evf);
                    }
                    for (auto const & dvf : valFeats.get_disabled())
                    {
                        accum.get<4>().push_back(dvf);
                    }
                }

                // Here we're identifying all the interesting criteria providers
                // that we'll possibly want to query about in device selection.
                for (auto devExt_c : criteria_c.get_deviceExtensions())
                {
                    accum.get<5>().push_back(devExt_c);
                }

                for (auto feat_c : criteria_c.get_features())
                {
                    accum.get<6>().push_back(feat_c.first);
                }

                for (auto prop_c : criteria_c.get_properties())
                {
                    accum.get<7>().push_back(prop_c.first);
                }
            }

            return std::make_tuple(ok, foundProfile);
        };


        bool ok = true;

        { // nested scope keeping me careful about not reusing ar
            AbilityResolver ar { aliases, abilities };
            for (auto inc_c : config_c.get_instanceInclude())
                { ar.include(inc_c); }

            if (auto const & criteria_c = config_c.get_sharedInstanceCriteria();
                criteria_c.has_value())
            {
                ok = ok && ar.doCrit(criteria_c->get_name(), * criteria_c, false, fn, accum, false);
            }
        }

        if (ok == false)
            { return false; }

        for (int i = 0; i < config_c.get_deviceProfileGroups().size(); ++i)
        {
            auto const & profileGroup_c = config_c.get_deviceProfileGroups()[i];
            auto mark = accum.mark();

            AbilityResolver ar { aliases, abilities };
            for (auto inc_c : profileGroup_c.get_include())
                { ar.include(inc_c); }

            auto & deviceInfo = deviceAssignments[i].expDeviceInfo;
            deviceInfo = expInstInfo;

            Accumulator devAccum(deviceInfo.instanceInfo.extensions, deviceInfo.instanceInfo.layers, deviceInfo.instanceInfo.debugMessengers,
                                 deviceInfo.instanceInfo.enabledValidation, deviceInfo.instanceInfo.disabledValidation,
                                 deviceInfo.deviceExtensions, deviceInfo.featureProviders, deviceInfo.propertyProviders);

            int profileIdx = ar.doProfileGroup(profileGroup_c.get_name(), profileGroup_c, false, fn, devAccum, false);
            if (profileIdx != NoGoodProfile)
            {
                accum.extend(devAccum);
            }
            else
            {
                devAccum.rollBack(mark);
            }
        }

        requireGlfwExtensions();
        consolidateExploratoryCollections();
    }

    bool DeviceCreator::requireGlfwExtensions()
    {
        // get GLFW extensions
        uint32_t numGlfwExts = 0;
        char const ** glfwExts = nullptr;
        if (app->anyVulkanWindowViews())
        {
            glfwExts = app->getVkExtensionsForGlfw(& numGlfwExts);
            for (uint32_t i = 0; i < numGlfwExts; ++i)
            {
                char const * extName = glfwExts[i];
                if (checkExtension(std::string_view {extName}, availableInstanceExtensionNames))
                    { expInstInfo.extensions.push_back(extName); }
                else
                {
                    log(fmt::format("Could not find necessary GLFW extension '{}'", extName));
                    return false;
                }
            }
        }
        return true;
    }


    void DeviceCreator::makeExploratoryInstance()
    {
        void * createInfo_pNext = makeDebugMessengersAndValidators(expInstInfo);

        VkApplicationInfo vai {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = appName_c.data(),
            .applicationVersion = VK_MAKE_API_VERSION(
                0, appVersion_c.major(), appVersion_c.minor(), appVersion_c.patch()),
            .pEngineName = "overground",
            .engineVersion = VK_MAKE_API_VERSION(
                0, Engine::version[0], Engine::version[1], Engine::version[2]
            ),
            .apiVersion = utilizedVulkanVersion.bits
        };

        VkInstanceCreateInfo vici {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = createInfo_pNext,
            .pApplicationInfo = & vai,
            .enabledLayerCount = static_cast<uint32_t>(expInstInfo.layers.size()),
            .ppEnabledLayerNames = expInstInfo.layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(expInstInfo.extensions.size()),
            .ppEnabledExtensionNames = expInstInfo.extensions.data()
        };

        VKR(vkCreateInstance(& vici, nullptr, & vkInstance));
        log("exploratory vulkan instance created.");
    }

    void DeviceCreator::matchDeviceAbilities()
    {
        AbilityResolver ar(aliases, abilities);

        for (int i = 0; i < config_c.get_deviceProfileGroups().size(); ++i)
        {
            auto const & dpg = config_c.get_deviceProfileGroups()[i];

//**************************************************
            Accumulator accum(...);

            auto fn = [this](crit const & criteria_c, decltype(accum) & accum)
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
                            { accum.get<0>().push_back(ext_c.data()); }
                    }
                }
                if (ok && criteria_c.get_layers().size() > 0)
                {
                    auto const & lays_c = criteria_c.get_extensions();
                    for (auto const & lay_c : lays_c)
                    {
                        ok = ok && checkLayer(lay_c, availableLayerNames);
                        if (ok)
                            { accum.get<1>().push_back(lay_c.data()); }
                    }
                }
                if (ok && criteria_c.get_debugUtilsMessengers().size() > 0)
                {
                    auto const & dums = criteria_c.get_debugUtilsMessengers();
                    for (auto const & dum : dums)
                    {
                        accum.get<2>().push_back(dum);
                    }
                }

                if (ok && criteria_c.get_validationFeatures().has_value())
                {
                    auto const & valFeats = * criteria_c.get_validationFeatures();
                    for (auto const & evf : valFeats.get_enabled())
                    {
                        accum.get<3>().push_back(evf);
                    }
                    for (auto const & dvf : valFeats.get_disabled())
                    {
                        accum.get<4>().push_back(dvf);
                    }
                }

                return std::make_tuple(ok, foundProfile);
            };


            bool ok = true;

            { // nested scope keeping me careful about not reusing ar
                AbilityResolver ar { aliases, abilities };
                for (auto inc_c : config_c.get_instanceInclude())
                    { ar.include(inc_c); }

                if (auto const & criteria_c = config_c.get_sharedInstanceCriteria();
                    criteria_c.has_value())
                {
                    ok = ok && ar.doCrit(criteria_c->get_name(), * criteria_c, false, fn, accum, false);
                }
            }

            if (ok == false)
                { return false; }

            for (auto const & profileGroup_c : config_c.get_deviceProfileGroups())
            {
                auto mark = accum.mark();

                AbilityResolver ar { aliases, abilities };
                for (auto inc_c : profileGroup_c.get_include())
                    { ar.include(inc_c); }

                int profileIdx = ar.doProfileGroup(profileGroup_c.get_name(), profileGroup_c, false, fn, accum, false);
                if (profileIdx == NoGoodProfile)
                    { accum.rollBack(mark); }
            }

//*********************************************

        }
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

    bool DeviceCreator::checkVulkan(std::string_view vulkanVersion, version_t available)
    {
        return version_t {vulkanVersion} .bits <= available.bits;
    }

    bool DeviceCreator::checkExtension(std::string_view extension, std::unordered_set<char const *> const & available)
    {
        return available.find(extension.data()) != end(available);
    }

    bool DeviceCreator::checkLayer(std::string_view layer, std::unordered_set<char const *> const & available)
    {
        return available.find(layer.data()) != end(available);
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
