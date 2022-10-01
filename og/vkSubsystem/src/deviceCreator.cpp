#include "../inc/deviceCreator.hpp"
#include "../../app/inc/app.hpp"
#include "../../logger/inc/logger.hpp"
#include "../../abilities/inc/abilityResolver.hpp"
#include "../../app/inc/troveKeeper.hpp"
#include <ranges>
#include <fmt/format.h>
#include <numeric>


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
    using critKinds = abilities::criteriaKinds;

    void * InstanceInfo::makeDebugMessengersAndValidators()
    {
        void * createInfo_pNext = nullptr;

        debugMessengerObjects.resize(debugMessengers.size());
        for (int i = 0; i < debugMessengerObjects.size(); ++i)
        {
            auto const & cfg_c = debugMessengers[i];
            debugMessengerObjects[i] = std::move(makeDebugMessengerCreateInfo(std::get<1>(cfg_c)));
            debugMessengerObjects[i].pNext = nullptr;
            if (i > 0)
                { debugMessengerObjects[i - 1].pNext = & debugMessengerObjects[i]; }
        }
        if (debugMessengerObjects.size() > 0)
            { createInfo_pNext = & debugMessengerObjects[0]; }

        if (enabledValidations.size() > 0 || disabledValidations.size() > 0)
        {
            validationFeatures = VkValidationFeaturesEXT {
                .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
                .pNext = createInfo_pNext,
                .enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidations.size()),
                .pEnabledValidationFeatures = enabledValidations.data(),
                .disabledValidationFeatureCount = static_cast<uint32_t>(disabledValidations.size()),
                .pDisabledValidationFeatures = disabledValidations.data()
            };
            createInfo_pNext = & (* validationFeatures);
        }

        return createInfo_pNext;
    }

    DeviceCreator::DeviceCreator(vkSubsystem::deviceConfig const & config_c, ProviderAliasResolver const & aliases, 
                                 AbilityCollection const & abilities, std::string_view appName_c, version_t appVersion_c)
    : config_c(config_c), aliases(aliases), abilities(abilities), appName_c(appName_c), appVersion_c(appVersion_c)
    {
    }

    /*
    VulkanSubsystem DeviceCreator::createInstanceAndDevices()
    {
        initPhysDevices();

        // vec of [groupIdx, numDevices]
        std::vector<std::tuple<int, int>> deviceSchedule;

        log("Rounding up device profile groups.");
        auto const & groups = config_c.get_deviceProfileGroups();

        auto getGroup = [&](std::string_view groupName)
        {
            auto groupIt = std::find_if(begin(groups), end(groups),
                                   [& groupName](auto & a){ return a.get_name() == groupName; });
            if (groupIt == end(groups))
                { throw Ex(fmt::format("Invalid vkDeviceProfileGroups group name '{}'", groupName)); }

            return groupIt - begin(groups);
        };

        for (auto const & work : appConfig.get_works())
        {
            for (auto const & [groupName, needed, wanted] : work.get_useDeviceProfileGroups())
            {
                auto groupIdx = getGroup(groupName);
                deviceSchedule.emplace_back(groupIdx, needed);
            }
        }

        for (auto const & work : appConfig.get_works())
        {
            for (auto const & [groupName, needed, wanted] : work.get_useDeviceProfileGroups())
            {
                auto groupIdx = getGroup(groupName);
                deviceSchedule.emplace_back(groupIdx, wanted - needed);
            }
        }

        log("Scoring devices for profile groups.");
        for (auto const & [groupIdx, _] : deviceSchedule)
        {
            computeBestProfileGroupDevices(groupIdx);
        }

        log("Assigning winning devices to profile groups.");
        for (auto const & [groupIdx, numDevices] : deviceSchedule)
        {
            assignDevices(groupIdx, numDevices);
        }

        log("Creating devices.");
        createAllVkDevices();
    }
    */
   
    InstanceSubsystem DeviceCreator::createVulkanSubsystem(
        std::vector<std::tuple<std::string_view, std::string_view, size_t>> const & schedule,
        std::vector<char const *> const & requiredExtensions,
        std::vector<char const *> const & requiredLayers)
    {
        std::unordered_set<std::string_view> alreadyMatched;
        std::vector<std::string_view> deviceGroups;
        for (auto [_0, deviceGroup, _1] : schedule)
        {
            auto [_, didInsert] = alreadyMatched.insert(deviceGroup);
            if (didInsert)
            {
                deviceGroups.push_back(deviceGroup);
            }
        }

        gatherExploratoryInstanceCriteria(deviceGroups, requiredExtensions, requiredLayers);
        makeExploratoryInstance();
        initPhysDevices();

        for (auto deviceGroup : deviceGroups)
        {
            matchDeviceAbilities(deviceGroup);
        }

        for (auto [engineName, deviceGroup, numPhysDevices] : schedule)
        {
            assignDevices(engineName, deviceGroup, numPhysDevices);
        }

        InstanceSubsystem vs;
        consolidateFinalCollections(vs);
        makeFinalInstance(vs);
        makeDevices(vs);
        makeQueues(vs);

        return vs;
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

    bool DeviceCreator::gatherExploratoryInstanceCriteria(
        std::vector<std::string_view> const & interestingDeviceGroups,
        std::vector<char const *> const & requiredExtensions,
        std::vector<char const *> const & requiredLayers)
    {
        // get vulkan runtime version
        uint32_t vulkanVersion = 0;
        VKR(vkEnumerateInstanceVersion(& vulkanVersion));
        availableVulkanVersion = version_t { vulkanVersion };
        if (availableVulkanVersion < config_c.get_minVulkanVersion())
            { Ex(fmt::format("Vulkan version supported ({}) does not meet the minimum specified ({}).",
                HumonFormat(availableVulkanVersion), HumonFormat(config_c.get_minVulkanVersion()))); }
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

        count = 0;
        vkEnumerateInstanceLayerProperties(& count, nullptr);
        availableLayers.resize(count);
        vkEnumerateInstanceLayerProperties(& count, availableLayers.data());
        for (int i = 0; i < count; ++i)
        {
            availableLayerNames.insert(
                availableLayers[i].layerName);
        }

        /*  The rest of this function is about getting all the providers
            and extensions that any ability in this device profile might
            want to use.
        */
        auto accum = expInstInfo.makeAccumulator();

        auto fn = [this](crit const & criteria_c, decltype(accum) & accum, auto _)
        {
            bool ok = true;
            bool foundProfile = false;

            if (criteria_c.get_vulkanVersion().has_value())
            {
                ok = ok && checkVulkan(* criteria_c.get_vulkanVersion(), utilizedVulkanVersion);
                foundProfile = ok;
            }
            for (auto const & ext_c : criteria_c.get_extensions())
            {
                ok = ok && checkExtension(ext_c, availableInstanceExtensionNames);
                if (ok)
                    { accum.get<0>().push_back(ext_c.data()); }
            }
            for (auto const & lay_c : criteria_c.get_layers())
            {
                ok = ok && checkLayer(lay_c, availableLayerNames);
                if (ok)
                    { accum.get<1>().push_back(lay_c.data()); }
            }

            if (ok)
            {
                for (auto const & ext_c : criteria_c.get_desiredExtensions())
                {
                    // optional; we're not setting ok here, just adding if it's available
                    if (checkExtension(ext_c, availableInstanceExtensionNames))
                        { accum.get<0>().push_back(ext_c.data()); }
                }
                for (auto const & dum : criteria_c.get_debugUtilsMessengers())
                    { accum.get<2>().emplace_back(criteria_c.get_name(), dum); }
                if (criteria_c.get_validationFeatures().has_value())
                {
                    auto const & valFeats = * criteria_c.get_validationFeatures();
                    for (auto const & evf : valFeats.get_enabled())
                        { accum.get<3>().push_back(evf); }
                    for (auto const & dvf : valFeats.get_disabled())
                        { accum.get<4>().push_back(dvf); }
                }
                // Here we're identifying all the interesting criteria providers
                // that we'll possibly want to query about in device selection.
                for (auto devExt_c : criteria_c.get_deviceExtensions())
                    { accum.get<5>().push_back(devExt_c.data()); }
                for (auto devExt_c : criteria_c.get_desiredDeviceExtensions())
                    { accum.get<5>().push_back(devExt_c.data()); }
                for (auto feat_c : criteria_c.get_features())
                    { accum.get<6>().emplace_back(feat_c.first, feat_c.second); }
                for (auto feat_c : criteria_c.get_desiredFeatures())
                    { accum.get<6>().emplace_back(feat_c.first, feat_c.second); }
                for (auto prop_c : criteria_c.get_properties())
                    { accum.get<7>().push_back(prop_c.first); }
                for (auto qfProp_c : criteria_c.get_queueFamilyProperties())
                    { accum.get<8>().push_back(qfProp_c.first); }
            }

            return std::make_tuple(ok, foundProfile);
        };

        bool ok = true;
        bool _ = false;

        {
            auto & ar = sharedInstanceAbilityResolver;
            ar.init(aliases, abilities);
            for (auto inc_c : config_c.get_instanceInclude())
                { ar.include(inc_c); }

            if (auto const & criteria_c = config_c.get_sharedInstanceCriteria();
                criteria_c.has_value())
            {
                ok = ok && ar.doCrit(criteria_c->get_name(), * criteria_c, false, fn, accum, _, false);
            }

            for (auto extName : requiredExtensions)
            {
                if (ok = ok && checkExtension(std::string_view {extName}, availableInstanceExtensionNames))
                    { expInstInfo.extensions.push_back(extName); }
                else
                {
                    log(fmt::format("Could not find necessary extension '{}'", extName));
                }
            }

            for (auto layerName : requiredLayers)
            {
                if (ok = ok && checkExtension(std::string_view {layerName}, availableLayerNames))
                    { expInstInfo.layers.push_back(layerName); }
                else
                {
                    log(fmt::format("Could not find necessary layer '{}'", layerName));
                }
            }
        }

        if (ok == false)
            { return false; }

        for (auto interestingDevGroupIdx = 0; interestingDevGroupIdx < interestingDeviceGroups.size(); ++interestingDevGroupIdx)
        {
            auto devGroupName = interestingDeviceGroups[interestingDevGroupIdx];
            auto devGroupIdx = getDeviceGroupIdx(devGroupName);

            auto const & profileGroup_c = config_c.get_deviceProfileGroups()[devGroupIdx];
            auto mark = accum.mark();

            auto & ar = deviceAssignments[devGroupIdx].abilityResolver;
            ar.init(aliases, abilities);
            for (auto inc_c : profileGroup_c.get_include())
                { ar.include(inc_c); }

            auto & deviceInfo = deviceAssignments[devGroupIdx].expDeviceInfo;

            auto && devAccum = deviceInfo.makeAccumulator();

            int profileIdx = ar.doProfileGroup(profileGroup_c.get_name(), profileGroup_c, false, fn, devAccum, _, false);
            if (profileIdx != NoGoodProfile)
            {
                accum.extend(devAccum);
            }
            else
            {
                devAccum.rollBack(mark);
            }

            for (auto const & qvProfile_c : profileGroup_c.get_queueVillageProfiles())
            {
                for (auto const & qfAttribs_c : qvProfile_c.get_queueVillage())
                {
                    auto const & crit = qfAttribs_c.get_criteria();
                    if (crit.has_value())
                    {
                        ar.doCrit(qfAttribs_c.get_name(), * qfAttribs_c.get_criteria(), false, fn, accum, _, false);
                    }
                }
            }
        }

        requireGlfwExtensions();
        consolidateExploratoryCollections();
    }

    void DeviceCreator::consolidateExploratoryCollections()
    {
        std::unordered_set<char const *> addedExts;
        std::unordered_set<char const *> addedLayers;
        std::unordered_set<std::string_view> addedDebugMessengers;
        std::unordered_set<VkValidationFeatureEnableEXT> addedEnabledValidations;
        std::unordered_set<VkValidationFeatureDisableEXT> addedDisabledValidations;
        std::unordered_set<char const *> addedDevExts;
        std::unordered_map<std::string_view, 
                            std::unordered_set<std::string_view>> addedFeatures;
        std::unordered_set<std::string_view> addedProperties;
        std::unordered_set<std::string_view> addedQueueFamilyProperties;

        auto & idi = expInstInfo;

        idi.extensions = makeUnique(idi.extensions);
        idi.layers = makeUnique(idi.layers);
        idi.debugMessengers = makeUnique(idi.debugMessengers);
        idi.enabledValidations = makeUnique(idi.enabledValidations);
        idi.disabledValidations = makeUnique(idi.disabledValidations);
        idi.deviceExtensions = makeUnique(idi.deviceExtensions);
        idi.features;
        for (auto const & [provider, feature] : idi.features)
        {
            std::unordered_set<std::string_view> newSet {};
            auto [itprov, _0] = addedFeatures.insert(std::make_pair(provider, newSet));
            auto [_1, didInsert] = itprov->second.insert(feature);
            if (didInsert)
                { idi.features.push_back(std::make_tuple(provider, feature)); }
        }
        idi.propertyProviders = makeUnique(idi.propertyProviders);
        idi.queueFamilyPropertyProviders = makeUnique(idi.queueFamilyPropertyProviders);
    }

    void DeviceCreator::makeExploratoryInstance()
    {
        void * createInfo_pNext = expInstInfo.makeDebugMessengersAndValidators();

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

    void DeviceCreator::initPhysDevices()
    {
        uint32_t physCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, & physCount, nullptr);
        std::vector<VkPhysicalDevice> phDevices(physCount);
        vkEnumeratePhysicalDevices(vkInstance, & physCount, phDevices.data());

        physDevices.resize(physCount);
        for (int physIdx = 0; physIdx < physCount; ++physIdx)
        {
            physDevices[physIdx].init(physIdx, phDevices[physIdx]);
        }

        auto const & profileGroups_c = config_c.get_deviceProfileGroups();
        deviceAssignments.resize(profileGroups_c.size());
    }

    void DeviceCreator::matchDeviceAbilities(std::string_view deviceGroup)
    {
        AbilityResolver ar;
        ar.init(aliases, abilities);

        int dpgIdx = getDeviceGroupIdx(deviceGroup);
        if (dpgIdx == -1)
            { return; }

        auto & devAss = deviceAssignments[dpgIdx];
        devAss.deviceSuitabilities.resize(physDevices.size());

        for (int physIdx = 0; physIdx < physDevices.size(); ++physIdx)
        {
            auto & physDevice = physDevices[physIdx].physicalDevice;
            auto const & availableDeviceExts = physDevices[physIdx].availableDeviceExtensions;
            auto & suitability = devAss.deviceSuitabilities[physIdx];

            suitability.physicalDeviceIdx = physIdx;

            //  get features, props, etc from device
            VkFeatures availableFeatures;
            availableFeatures.init(devAss.get_expFeatureProviders());
            VkProperties availableProperties;
            availableProperties.init(devAss.get_expPropertyProviders());
            vkGetPhysicalDeviceFeatures2(physDevice, & availableFeatures.mainStruct);
            vkGetPhysicalDeviceProperties2(physDevice, & availableProperties.mainStruct);

            //  check device against features, props, etc
            suitability.bestProfileIdx = -1;
            do
            {
                suitability.bestProfileIdx = getBestProfile(dpgIdx, physIdx, availableFeatures,
                    availableProperties, suitability.bestProfileIdx + 1);
                if (suitability.bestProfileIdx >= 0)
                {
                    VkQueueFamilies availableQfProperties;
                    availableQfProperties.init(devAss.get_expQueueFamilyPropertyProviders());
                    suitability.bestQueueVillageProfile = getBestQueueVillageProfile(dpgIdx, physIdx,
                        availableFeatures, availableProperties, availableQfProperties);
                    if (suitability.bestQueueVillageProfile < 0)
                        { suitability.bestProfileIdx = NoGoodProfile; }
                }
            } while (suitability.bestProfileIdx < 0);

            //  if we found a profile:
            //      da.deviceProfiles[physDevIdx].bestProfileIdx = profileIdx
            //      da.deviceProfiles[physDevIdx].usedProviders = proUsed
        }
    }

    //static auto checkCriteria(
    std::tuple<bool, bool> DeviceCreator::checkCriteria(
        VkFeatures const & availbleFeatures, VkProperties const & availableProperties, 
        PhysVkDevice & physDev, crit const & criteria_c, decltype(InstanceDeviceInfo().makeAccumulator()) & accum, auto _)
    {
        bool ok = true;
        bool foundProfile = true;

        if (ok && criteria_c.get_vulkanVersion().has_value())
        {
            ok = ok && checkVulkan(* criteria_c.get_vulkanVersion(), utilizedVulkanVersion);
            foundProfile = foundProfile && ok;
        }
        if (ok && criteria_c.get_extensions().size() > 0)
        {
            auto const & exts_c = criteria_c.get_extensions();
            for (auto const & ext_c : exts_c)
            {
                if (ok = ok && checkExtension(ext_c, availableInstanceExtensionNames))
                    { accum.get<0>().push_back(ext_c.data()); }
                foundProfile = foundProfile && ok;
            }
        }
        if (ok && criteria_c.get_layers().size() > 0)
        {
            auto const & lays_c = criteria_c.get_layers();
            for (auto const & lay_c : lays_c)
            {
                if (ok = ok && checkLayer(lay_c, availableLayerNames))
                    { accum.get<1>().push_back(lay_c.data()); }
                foundProfile = foundProfile && ok;
            }
        }
        if (ok && criteria_c.get_deviceExtensions().size() > 0)
        {
            for (auto devExt_c : criteria_c.get_deviceExtensions())
            {
                if (ok = ok && checkDeviceExtension(devExt_c, physDev.availableDeviceExtensions))
                    { accum.get<5>().push_back(devExt_c.data()); }
                foundProfile = foundProfile && ok;
            }
        }
        if (ok && criteria_c.get_features().size() > 0)
        {
            for (auto feat_c : criteria_c.get_features())
            {
                auto const & [provider_c, features_c] = feat_c;
                for (auto feature_c : features_c)
                {
                    if (ok = ok && checkFeature(provider_c, feature_c, availbleFeatures))
                        { accum.get<6>().emplace_back(provider_c, feature_c); }
                    foundProfile = foundProfile && ok;
                }
            }
        }
        if (ok && criteria_c.get_properties().size() > 0)
        {
            for (auto prop_c : criteria_c.get_properties())
            {
                auto const & [provider_c, properties_c] = prop_c;
                if (ok = ok && checkProperties(provider_c, properties_c, availableProperties))
                    { accum.get<7>().push_back(prop_c.first); }
                foundProfile = foundProfile && ok;
            }
        }
        if (ok)
        {
            if (criteria_c.get_debugUtilsMessengers().size() > 0)
            {
                auto const & dums = criteria_c.get_debugUtilsMessengers();
                for (auto const & dum : dums)
                    { accum.get<2>().push_back({criteria_c.get_name(), dum}); }
            }

            if (criteria_c.get_validationFeatures().has_value())
            {
                auto const & valFeats = * criteria_c.get_validationFeatures();
                for (auto const & evf : valFeats.get_enabled())
                    { accum.get<3>().push_back(evf); }
                for (auto const & dvf : valFeats.get_disabled())
                    { accum.get<4>().push_back(dvf); }
            }
            for (auto ext_c : criteria_c.get_desiredExtensions())
            {
                if (checkExtension(ext_c, availableInstanceExtensionNames))
                    { accum.get<0>().push_back(ext_c.data()); }
            }
            for (auto devExt_c : criteria_c.get_desiredDeviceExtensions())
            {
                if (checkDeviceExtension(devExt_c, physDev.availableDeviceExtensions))
                    { accum.get<5>().push_back(devExt_c.data()); }
            }
            for (auto const & feat_c : criteria_c.get_desiredFeatures())
            {
                auto const & [provider_c, features_c] = feat_c;
                for (auto feature_c : features_c)
                {
                    if (checkFeature(provider_c, feature_c, availbleFeatures))
                        { accum.get<6>().emplace_back(provider_c, feature_c); }
                }
            }
        }

        return std::make_tuple(ok, foundProfile);
    }

    int DeviceCreator::getBestProfile(int devGroupIdx, int physDevIdx, VkFeatures const & availbleFeatures,
                                      VkProperties const & availableProperties, int startingProfileIdx)
    {
        auto const & profileGroup_c = config_c.get_deviceProfileGroups()[devGroupIdx];
        auto & devAss = deviceAssignments[devGroupIdx];
        auto & devSuit = devAss.deviceSuitabilities[physDevIdx];
        auto & physDev = physDevices[physDevIdx];

        auto accum = devSuit.bestProfileDeviceInfo.makeAccumulator();
        auto fn = [this, & availbleFeatures, & availableProperties, & physDev]
                    (crit const & criteria_c, decltype(accum) & accum, auto _)
        {
            for (auto const & qfProp_c : criteria_c.get_queueFamilyProperties())
            {
                accum.get<8>().push_back(qfProp_c.first);
            }

            return checkCriteria(availbleFeatures, availableProperties, physDev, criteria_c, accum, _);
        };

        bool ok = true;
        bool _ = false;

        auto mark = accum.mark();

        { // nested scope keeping me careful about not reusing ar
            auto & ar = sharedInstanceAbilityResolver;
            for (auto inc_c : config_c.get_instanceInclude())
                { ar.include(inc_c); }

            if (auto const & criteria_c = config_c.get_sharedInstanceCriteria();
                criteria_c.has_value())
            {
                ok = ok && ar.doCrit(criteria_c->get_name(), * criteria_c, false, fn, accum, _, false);
            }
        }

        if (ok == false)
            { return NoGoodProfile; }

        auto & ar = devAss.abilityResolver;
        for (auto inc_c : profileGroup_c.get_include())
            { ar.include(inc_c); }

        int profileIdx = ar.doProfileGroup(profileGroup_c.get_name(), profileGroup_c, false,
                                           fn, accum, _, false, startingProfileIdx);
        if (profileIdx != NoGoodProfile)
        {
            return profileIdx;
        }

        accum.rollBack(mark);
        return NoGoodProfile;
    }

    int DeviceCreator::getBestQueueVillageProfile(int devGroupIdx, int physDevIdx,
        VkFeatures const & availbleFeatures, VkProperties const & availableProperties,
        VkQueueFamilies const & availableQfProperties)
    {

        auto const & profileGroup_c = config_c.get_deviceProfileGroups()[devGroupIdx];
        auto const & qvProfiles_c = profileGroup_c.get_queueVillageProfiles();
        auto & devAss = deviceAssignments[devGroupIdx];
        auto & devSuit = devAss.deviceSuitabilities[physDevIdx];
        int bestDevProfileIdx = devSuit.bestProfileIdx;
        auto & physDev = physDevices[physDevIdx];

        auto accum = devSuit.bestProfileDeviceInfo.makeAccumulator();

        uint32_t devQueueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(physDev.physicalDevice, & devQueueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties2> qfPropsVect(devQueueFamilyCount);
        devSuit.queueFamilies.resize(devQueueFamilyCount);
        for (int i = 0; i < devQueueFamilyCount; ++i)
        {
            devSuit.queueFamilies[i] = availableQfProperties.copyAndReset();
            qfPropsVect[i] = devSuit.queueFamilies[i].mainStruct;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(physDev.physicalDevice, & devQueueFamilyCount, qfPropsVect.data());
        for (int i = 0; i < devQueueFamilyCount; ++i)
        {
            devSuit.queueFamilies[i].mainStruct = qfPropsVect[i];
        }

        auto check = [this, & availbleFeatures, & availableProperties, & availableQfProperties, & physDev]
                    (crit const & criteria_c, decltype(accum) & accum, auto _)
        {
            bool ok = true;
            bool foundProfile = true;

            // TODO HERE: Check on queue family properties, and then 
            // pass through to checkCriteria().
            if (ok && criteria_c.get_queueFamilyProperties().size() > 0)
            {

                for (auto qfProp_c : criteria_c.get_queueFamilyProperties())
                {
                    auto const & [provider_c, qfProperties_c] = qfProp_c;
                    if (ok = ok && checkQueueFamilyProperties(provider_c, qfProperties_c, availableQfProperties))
                        { accum.get<8>().push_back(qfProp_c.first); }
                    foundProfile = foundProfile && ok;
                }
            }

            if (ok)
                { return checkCriteria(availbleFeatures, availableProperties, physDev, criteria_c, accum, _); }
            else
                { return { false, false}; }
        };

        auto & suitabilityQfs = devSuit.queueFamilies;
        devSuit.bestQueueVillageProfile = -1;
        for (uint32_t qvProfileIdx_c = 0; qvProfileIdx_c < qvProfiles_c.size(); ++qvProfileIdx_c)
        {
            auto villageMark = accum.mark();

            auto const & qvProfile_c = qvProfiles_c[qvProfileIdx_c];
            auto const & qvillage_c = qvProfile_c.get_queueVillage();
            log(fmt::format(". . . . checking qfprofile {} (index #{})", qvProfile_c.get_name(), qvProfileIdx_c));

            if (suitabilityQfs.size() < qvillage_c.size())
            {
                log(fmt::format(". . . . . Queue family profile {} - no match (too few queue families on device).", qvProfileIdx_c));
                continue;
            }

            bool qfProfileFail = false;
            // Populate a list of qfis that match the queue spec for each queue.
            // For each queueFamily_c, a list of qfis that meet reqs.
            auto selectableQueueFamilyIndices = std::vector<std::vector<uint32_t>> (qvillage_c.size());
            for (uint32_t qfaIdx_c = 0; qfaIdx_c < qvillage_c.size(); ++qfaIdx_c)
            {
                log(fmt::format(". . . . . checking family {}", qfaIdx_c));

                auto qfaMark = accum.mark();

                auto const & qfAttribs_c = qvillage_c[qfaIdx_c];
                // starts empty; will collect each qfi that meets the requirements
                auto & selectable = selectableQueueFamilyIndices[qfaIdx_c];
                for (int devQfi = 0; devQfi < suitabilityQfs.size(); ++devQfi)
                {
                    bool ok = false;
                    auto qfiMark = accum.mark();

                    if (qfAttribs_c.get_criteria().has_value())
                    {
                        bool _ = false;
                        auto & ar = devAss.abilityResolver;
                        ok = ok && ar.doCrit(qfAttribs_c.get_name(), * qfAttribs_c.get_criteria(), false, check, accum, _, false);
                    }
                    if (ok)
                    {
                        log(fmt::format(". . . . . . qf index {} is selectable", devQfi));
                        selectable.push_back(devQfi);
                    }
                    else
                    {
                        log(fmt::format(". . . . . . qf index {} is not selectable", devQfi));
                        accum.rollBack(qfiMark);
                    }
                }

                if (selectable.size() == 0)
                {
                    accum.rollBack(qfaMark);
                    qfProfileFail = true;
                }
            }

            if (qfProfileFail)
            {
                log(fmt::format(". . . . . Queue family profile {} - no match.", qvProfileIdx_c));
                accum.rollBack(villageMark);
                continue;
            }

            /*  At this point, selectableQueueFamilyIndices has at least one 
                qfi in each of its vectors. Now we try to find vectors of qfis
                which are mutually unique, each element of which comes from 
                selectableQueueFamilyIndices. For instance,
                
                selectableQueueFamilyIndices: [
                        [ 0, 1, 2 ] // these qfis can work for qf 0
                        [ 2, 3 ]    // these qfis can work for qf 1
                        [ 0, 4, 5 ] // these qfis can work fro qf 2
                    ]
                unique entries are: [
                    [0, 2, 4]
                    [0, 2, 5]
                    [0, 3, 4]
                    [0, 3, 5]
                    [1, 2, 0]
                    [1, 2, 4]
                    [1, 2, 5]
                    [1, 3, 0]
                    [1, 3, 4]
                    [1, 3, 5]
                    [2, 3, 0]
                    [2, 3, 4]
                    [2, 3, 5]
                ]

                Each of these is in turn scored, and the best fitting one is
                selected as the winner.
            */
            
            auto numCombos = std::accumulate(begin(selectableQueueFamilyIndices), end(selectableQueueFamilyIndices),
                1, [](uint32_t b, auto & sub){ return static_cast<uint32_t>(sub.size()) * b; });
            auto comboScores = std::vector<uint32_t>(numCombos, 0);
            uint32_t winningScore = 0;
            uint32_t winningCombo = -1;

            // Now find the best unique qfi assignment for each queue.
            // If there is not one, go to next qfprofile.
            //std::vector<QueueFamilyAssignment> qfiAllocs;
            std::vector<uint32_t> qfiStack;
            std::vector<uint32_t> winningQfis;

            // This monsta lets us recurse to n dimensions to score up a village.
            // We keep a running score, which gets zeroed out for illegal combos.
            // TODO: This doesn't need to be recursive or a lambda.
            auto scoreVillage = [&](uint32_t famIdx, std::bitset<64> busyFamilies, uint32_t & comboIdx, uint64_t score) -> void
            {
                auto scoreVillage_int = [&](uint32_t famIdx, std::bitset<64> busyFamilies, uint32_t & comboIdx, uint64_t score, auto recurse) -> void
                {
                    auto const & qfis = selectableQueueFamilyIndices[famIdx];
                    for (uint32_t qfi : qfis)
                    {
                        qfiStack.push_back(qfi);

                        if (busyFamilies[qfi])
                        {
                            score = 0;
                        }                        
                        else
                        {
                            auto numQueuesOnDevice = suitabilityQfs[qfi].mainStruct.queueFamilyProperties.queueCount;
                            auto numQueuesDesired = qvProfile_c.get_queueVillage()[famIdx].get_maxQueueCount();
                            score *= std::min(numQueuesOnDevice, numQueuesDesired);
                            busyFamilies[qfi] = true;
                        }

                        // if this was a last one to check
                        if (famIdx + 1 == selectableQueueFamilyIndices.size())
                        {
                            comboScores[comboIdx] = score;
                            if (score > winningScore)
                            {
                                winningScore = score;
                                winningCombo = comboIdx;
                                winningQfis = qfiStack;
                                log(fmt::format("@ leading combo: {} score {}", winningCombo, winningScore));
                            }
                            comboIdx += 1;
                        }
                        else
                        {
                            recurse(famIdx + 1, busyFamilies & std::bitset<64> { 1ULL << qfi }, comboIdx, score, recurse);
                        }
    
                        qfiStack.pop_back();
                    }
                };

                scoreVillage_int(famIdx, busyFamilies, comboIdx, score, scoreVillage_int);
            };

            uint32_t comboIdx = 0;
            scoreVillage(0, 0, comboIdx, 1);

            if (winningCombo != -1)
            {
                devSuit.bestQueueVillageProfile = qvProfileIdx_c;

                log(fmt::format("@ winning combo: {}", winningCombo));
                
                devSuit.queueFamilyAssignments.resize(winningQfis.size());
                for (int allocIdx = 0; allocIdx < winningQfis.size(); ++allocIdx)
                {
                    auto & alloc = devSuit.queueFamilyAssignments[allocIdx];
                    alloc.qfi = winningQfis[allocIdx];

                    auto const & qf_c = qvProfile_c.get_queueVillage()[allocIdx];

                    auto numQueuesOnDevice = suitabilityQfs[alloc.qfi].mainStruct.queueFamilyProperties.queueCount;
                    auto numQueuesDesired = qf_c.get_maxQueueCount();
                    alloc.count = std::min(numQueuesOnDevice, numQueuesDesired);
                    alloc.flags = qf_c.get_flags();

                    std::vector<float> ctdPriorities(alloc.count);
                    auto const & priorities_c = qf_c.get_priorities();
                    for (int i = 0; i < alloc.count; ++i)
                    {
                        ctdPriorities[i] = priorities_c[i % (priorities_c.size())];
                    }
                    alloc.priorities = std::move(ctdPriorities);
                    alloc.globalPriority = qf_c.get_globalPriority();
                }

                // we found a winner and recorded the qfi data, now bail
                break;
            }

            // no winner; we move to the next village profile
            accum.rollBack(villageMark);
        }

        if (devSuit.bestQueueVillageProfile < 0)
        {
            log(fmt::format(". Could not find a suitable queue family combination for profile group '{}'.", profileGroup_c.get_name()));
            // Not necessarily an error; a given profile group may just be a nice-to-have (require 0 devices).
            return -1;
        }

        log(fmt::format(". Best suitable queue village profile found: {} (profile #{})",
            qvProfiles_c[devSuit.bestQueueVillageProfile].get_name(),
            devSuit.bestQueueVillageProfile));

        return devSuit.bestQueueVillageProfile;
    }

    void DeviceCreator::assignDevices(std::string_view engineName, std::string_view groupName, int numDevices)
    {
        int groupIdx = getDeviceGroupIdx(groupName);
        auto const & profileGroups_c = config_c.get_deviceProfileGroups();
        auto const & group_c = profileGroups_c[groupIdx];
        auto & deviceAssignmentGroup = deviceAssignments[groupIdx];

        // compute the best numDevices devices for this device group.
        for (int numAssignments = 0; numAssignments < numDevices; ++numAssignments)
        {
            int bestProfileIdx = group_c.get_profiles().size();
            int bestDeviceIdx = -1;
            VkFeatures * bestDeviceFeatures_p = nullptr;
            // each time through, find the best available device for this group.
            for (int devIdx = 0; devIdx < physDevices.size(); ++devIdx)
            {
                auto & device = physDevices[devIdx];
                if (device.isAssignedToDeviceProfileGroup)
                    { continue; }

                auto & suitability = deviceAssignmentGroup.deviceSuitabilities[devIdx];
                if (suitability.bestProfileIdx == -1 ||
                    suitability.bestQueueVillageProfile == -1)
                    { continue; }

                if (suitability.bestProfileIdx < bestProfileIdx)
                {
                    bestProfileIdx = suitability.bestProfileIdx;
                    bestDeviceIdx = devIdx;
                    bestDeviceFeatures_p = & suitability.bestProfileFeatures;
                }
            }

            if (bestDeviceIdx == -1)
                { return; } // if any assignment fails, they'll fail every time, so bail now

            auto & device = physDevices[bestDeviceIdx];
            device.isAssignedToDeviceProfileGroup = true;
            device.groupIdx = groupIdx;
            device.profileIdx = bestProfileIdx;
            deviceAssignmentGroup.winningDeviceIdxs.push_back(std::make_tuple(engineName, bestDeviceIdx));
        }
    }

    void DeviceCreator::consolidateFinalCollections(InstanceSubsystem & vs)
    {
        /*  
            init instance accumulator sets (exts, layers)

            for each deviceGroup:
                if deviceAssignments[groupIdx].winningDeviceIdxs.size() == 0:
                    // this deviceGroup has no suitable physical devices
                    continue
                
                for each physDevIdx in deviceAssignments[groupIdx].winningDeviceIdxs:
                    init device accumulator sets (devexts, features)
                    add instance accumulations to device accumulators
                    auto suit & = deviceAssignments[groupIdx].deviceSuitabilities[physDevIdx]
                    DeviceInfo & di = suit.bestProfileDeviceInfo
                    gather di.extensions, layers into instance accumulators
                    gather di.* into device accumulator sets
        */
        vs.info.vulkanVersion = utilizedVulkanVersion;

        std::unordered_set<char const *> addedExts;
        std::unordered_set<char const *> addedLayers;
        std::unordered_set<std::string_view> addedDebugMessengers;
        std::unordered_set<VkValidationFeatureEnableEXT> addedEnabledValidations;
        std::unordered_set<VkValidationFeatureDisableEXT> addedDisabledValidations;

        for (int dai = 0; dai < deviceAssignments.size(); ++dai)
        {
            auto & ass = deviceAssignments[dai];
            for (auto [engineName, physDevIdx] : ass.winningDeviceIdxs)
            {
                std::unordered_set<char const *> addedDevExts;
                std::unordered_map<std::string_view, 
                                   std::unordered_set<std::string_view>> addedFeatures;
                std::unordered_set<std::string_view> addedProperties;
                std::unordered_set<std::string_view> addedQueueFamilyProperties;

                auto & suit = ass.deviceSuitabilities[physDevIdx];
                auto & idi = suit.bestProfileDeviceInfo;

                vs.devices.push_back(DeviceSubsystem {});
                auto & ds = vs.devices[vs.devices.size() - 1];
                ds.engineName = engineName;

                // Okay to copy; ds.abilityResolver.abilities normally would have 
                // to be rebuilt, but at this point we haven't been caching anything 
                // anyway, so it should be empty.
                assert(ass.abilityResolver.getCachesize() == 0);
                ds.abilityResolver = ass.abilityResolver;

                ds.physicalDeviceIdx = physDevIdx;
                ds.deviceGroupIdx = dai;
                ds.deviceGroupName = config_c.get_deviceProfileGroups()[dai].get_name();
                
                std::vector<std::string_view> fps;
                for (auto const & [fp, _0] : ass.expDeviceInfo.features)
                    { fps.push_back(fp); }
                ds.features.init(fps);

                vs.info.extensions = makeUnique(idi.extensions);
                vs.info.layers = makeUnique(idi.layers);
                vs.info.debugMessengers = makeUnique(idi.debugMessengers);
                vs.info.enabledValidations = makeUnique(idi.enabledValidations);
                vs.info.disabledValidations = makeUnique(idi.disabledValidations);
                ds.info.deviceExtensions = makeUnique(idi.deviceExtensions);

                for (auto const & [provider, feature] : idi.features)
                {
                    std::unordered_set<std::string_view> newSet {};
                    auto [itprov, _1] = addedFeatures.insert(std::make_pair(provider, newSet));
                    auto [_2, didInsert] = itprov->second.insert(feature);
                    if (didInsert)
                        { ds.info.features.push_back(std::make_tuple(provider, feature));
                          ds.features.set(provider, feature, VK_TRUE); }
                }

                ds.info.propertyProviders = makeUnique(idi.propertyProviders);
                ds.info.queueFamilyPropertyProviders = makeUnique(idi.queueFamilyPropertyProviders);

                // while we're here, record qfi stuff to ds
                for (auto const & qfa : suit.queueFamilyAssignments)
                {
                    ds.queueFamilies.push_back( QueueFamilySubsystem {
                        qfa.qfi,
                        qfa.count,
                        qfa.priorities,
                        qfa.flags,
                        qfa.globalPriority
                    });
                }
            }
        }
    }

    void DeviceCreator::makeFinalInstance(InstanceSubsystem & vs)
    {
        // TODO: Consider caching the interesting providers to prevent
        //       double-instancing every time.

        // Don't bother recreating if the extensions and layers are identical
        if (containTheSame(expInstInfo.extensions, vs.info.extensions)
         && containTheSame(expInstInfo.layers, vs.info.layers))
            { return; }

        // kill the exploratory shit if we are recreating.
        vkDestroyInstance(vkInstance, nullptr);
        vkInstance = nullptr;
        
        void * createInfo_pNext = vs.info.makeDebugMessengersAndValidators();

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
            .enabledLayerCount = static_cast<uint32_t>(vs.info.layers.size()),
            .ppEnabledLayerNames = vs.info.layers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(vs.info.extensions.size()),
            .ppEnabledExtensionNames = vs.info.extensions.data()
        };

        VKR(vkCreateInstance(& vici, nullptr, & vkInstance));
        log("final vulkan instance created.");

        initPhysDevices();
    }

    void DeviceCreator::makeDevices(InstanceSubsystem & vs)
    {
        for (int dsIdx = 0; dsIdx < vs.devices.size(); ++dsIdx)
        {
            DeviceSubsystem & ds = vs.devices[dsIdx];

            ds.features.init(getElement<0>(expInstInfo.features));
            ds.properties.init(expInstInfo.propertyProviders);
            vkGetPhysicalDeviceFeatures2(ds.physicalDevice, & ds.features.mainStruct);
            vkGetPhysicalDeviceProperties2(ds.physicalDevice, & ds.properties.mainStruct);
            
            // get VkQueueFamilies
            uint32_t devQueueFamilyCount = ds.queueFamilies.size();
            std::vector<VkQueueFamilyProperties2> qfPropsVect(devQueueFamilyCount);
            for (int i = 0; i < devQueueFamilyCount; ++i)
            {
                VkQueueFamilies & vkqf = ds.queueFamilies[i].queueFamilyProperties;
                vkqf.init(expInstInfo.queueFamilyPropertyProviders);
                qfPropsVect[i] = vkqf.mainStruct;
            }
            vkGetPhysicalDeviceQueueFamilyProperties2(ds.physicalDevice, & devQueueFamilyCount, qfPropsVect.data());

            // make the VkDeviceQueueCreateInfos
            auto dqcis = std::vector<VkDeviceQueueCreateInfo>(ds.queueFamilies.size());
  
            std::vector<VkDeviceQueueGlobalPriorityCreateInfoKHR> dqgpcis;
            for (int i = 0; i < devQueueFamilyCount; ++i)
            {
                auto & qfa = ds.queueFamilies[i];

                qfa.queueFamilyProperties.mainStruct = qfPropsVect[i];

                void * pNext = NULL;
                if (qfa.globalPriority.has_value())
                {
                    dqgpcis.push_back(VkDeviceQueueGlobalPriorityCreateInfoKHR { VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_KHR, NULL, VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR });
                    auto & globalPriorityStruct = dqgpcis[dqgpcis.size() - 1];
                    pNext = & globalPriorityStruct;
                    globalPriorityStruct.globalPriority = * qfa.globalPriority;
                }

                dqcis[i] =
                {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext = pNext,
                    .flags = qfa.flags,
                    .queueFamilyIndex = qfa.qfi,
                    .queueCount = qfa.count,
                    .pQueuePriorities = qfa.priorities.data()
                };
            }

            VkDeviceCreateInfo dci =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .pNext = & ds.features.mainStruct,
                .flags = 0,
                .queueCreateInfoCount = static_cast<uint32_t>(dqcis.size()),
                .pQueueCreateInfos = dqcis.data(),
                .enabledLayerCount = static_cast<uint32_t>(vs.info.layers.size()),
                .ppEnabledLayerNames = vs.info.layers.data(),
                .enabledExtensionCount = static_cast<uint32_t>(ds.info.deviceExtensions.size()),
                .ppEnabledExtensionNames = ds.info.deviceExtensions.data(),
                .pEnabledFeatures = NULL
            };

            VKR(vkCreateDevice(ds.physicalDevice, & dci, nullptr, & ds.device));
            log("IT WORKED");
        }
    }

    void DeviceCreator::makeQueues(InstanceSubsystem & vs)
    {
        // TODO: this
    }


    int DeviceCreator::getDeviceGroupIdx(std::string_view groupName)
    {
        int profileGroupIdx = -1;

        for (int dpgIdx = 0; dpgIdx < config_c.get_deviceProfileGroups().size(); ++dpgIdx)
        {
            auto const & profileGroup_c = config_c.get_deviceProfileGroups()[dpgIdx];
            if (profileGroup_c.get_name() != groupName)
                { continue; }
                
            profileGroupIdx = dpgIdx;
            break;
        }

        return profileGroupIdx;
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

    bool DeviceCreator::checkDeviceExtension(std::string_view extension, std::unordered_set<char const *> const & available)
    {
        return available.find(extension.data()) != end(available);
    }

    bool DeviceCreator::checkFeature(std::string_view provider_c, std::string_view feature_c, VkFeatures const & available)
    {
        return available.check(provider_c, feature_c);
    }

    bool DeviceCreator::checkProperties(std::string_view provider_c, std::vector<std::tuple<std::string_view, og::abilities::op, std::string_view>> const & properties_c, VkProperties const & available)
    {
        for (auto const & [property_c, op_c, value_c] : properties_c)
        {
            if (available.check(provider_c, property_c, op_c, value_c) == false)
                { return false; }
        }
        return true;
    }

    bool DeviceCreator::checkQueueFamilyProperties(std::string_view provider_c, std::vector<std::tuple<std::string_view, og::abilities::op, std::string_view>> const & qfProperties_c, VkQueueFamilies const & available)
    {
        for (auto const & [qfProperty_c, op_c, value_c] : qfProperties_c)
        {
            if (available.check(provider_c, qfProperty_c, op_c, value_c) == false)
                { return false; }
        }
        return true;
    }


    /*

    void DeviceCreator::computeBestProfileGroupDevices(int groupIdx)
    {
        auto const & profileGroups_c = config_c.get_deviceProfileGroups();
        auto const & group_c = profileGroups_c[groupIdx];
        auto const & profiles_c = group_c.get_profiles();

        log(fmt::format(". Scoring device group {}:", profileGroups_c[groupIdx].get_name()));

        auto & deviceAssignmentGroup = deviceAssignments[groupIdx];
        if (deviceAssignmentGroup.hasBeenComputed)
            { return; }

        deviceAssignmentGroup.deviceSuitabilities.resize(physDevices.size());

        for (int devIdx = 0; devIdx < physDevices.size(); ++devIdx)
        {
            log(fmt::format(". . Scoring physical device {}:", devIdx));

            auto & device = physDevices[devIdx];
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
                suitability.bestQueueVillageProfile = queueFamilyGroupIdx;
                if (queueFamilyGroupIdx >= 0)
                {
                    suitability.queueFamilyAssignments = std::move(queueFamilyAlloc);
                }
            }
        }
        deviceAssignmentGroup.hasBeenComputed = true;
    }
    */
   /*

    void DeviceCreator::createAllVkDevices()
    {
        for (int devIdx = 0; devIdx < physDevices.size(); ++devIdx)
        {
            auto & device = physDevices[devIdx];
            device.createVkDevice();
        }
    }

    void DeviceCreator::destroyAllDevices()
    {
        for (int i = 0; i < physDevices.size(); ++i)
        {
            destroyDevice(i);
        }
    }

    void DeviceCreator::destroyDevice(int deviceIdx)
    {
        physDevices[deviceIdx].destroyVkDevice();
    }
    */
}
