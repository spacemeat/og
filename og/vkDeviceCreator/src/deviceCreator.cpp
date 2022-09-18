#include "../inc/deviceCreator.hpp"
#include "../../engine/inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../inc/app.hpp"
#include "../../abilities/inc/abilityResolver.hpp"
#include "../../app/inc/troveKeeper.hpp"
#include <ranges>


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

    void * InstanceDeviceInfo::makeDebugMessengersAndValidators()
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

    void InstanceDeviceInfo::consolidateCollections()
    {
        uniquifyVectorOfThings(extensions);
        uniquifyVectorOfThings(layers);
        uniquifyVectorOfThings(enabledValidations);
        uniquifyVectorOfThings(disabledValidations);
    }


    DeviceCreator::DeviceCreator(std::string const & configPath, ProviderAliasResolver & aliases, AbilityCollection & abilities,
                                 std::string_view appName_c, version_t appVersion_c)
    : aliases(aliases), abilities(abilities), appName_c(appName_c), appVersion_c(appVersion_c)
    {
        config_c = vkDeviceCreator::deviceConfig { troves->loadAndKeep(configPath) };
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

    VulkanSubsystem DeviceCreator::createVulkanSubsystem(std::vector<std::tuple<std::string_view, size_t>> const & schedule)
    {
        VulkanSubsystem vs;
        gatherExploratoryInstanceExtensions();
        makeExploratoryInstance();
        initExploratoryPhysDevices();

        std::unordered_set<std::string_view> alreadyMatched;
        for (auto [deviceGroup, _] : schedule)
        {
            auto [_, didInsert] = alreadyMatched.insert(deviceGroup);
            if (didInsert)
            {
                matchDeviceAbilities(deviceGroup);
            }
        }

        for (auto [deviceGroup, numPhysDevices] : schedule)
        {
            assignDevices(deviceGroup, numPhysDevices);
        }

        alreadyMatched.clear();
        for (auto [deviceGroup, _] : schedule)
        {
            auto [_, didInsert] = alreadyMatched.insert(deviceGroup);
            if (didInsert)
            {
                gatherFinalCreationSet(deviceGroup, vs);
            }
        }

        makeFinalInstance(vs);
        makeDevices(vs);

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

        count = 0;
        vkEnumerateInstanceLayerProperties(& count, nullptr);
        availableLayers.resize(count);
        vkEnumerateInstanceLayerProperties(& count, availableLayers.data());
        for (int i = 0; i < count; ++i)
        {
            availableLayerNames.insert(
                availableLayers[i].layerName);
        }

        /*  The rest of this function is about getting all the instance
            extensions that any ability in this device profile might want to use.
        */
        auto accum = expInstInfo.makeAccumulator();

        auto fn = [this](crit const & criteria_c, decltype(accum) & accum, auto _)
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
                auto const & lays_c = criteria_c.get_layers();
                for (auto const & lay_c : lays_c)
                {
                    ok = ok && checkLayer(lay_c, availableLayerNames);
                    if (ok)
                        { accum.get<1>().push_back(lay_c.data()); }
                }
            }

            if (ok)
            {
                if (criteria_c.get_desiredExtensions().size() > 0)
                {
                    auto const & exts_c = criteria_c.get_desiredExtensions();
                    for (auto const & ext_c : exts_c)
                    {
                        // optional; we're not setting ok here, just adding if it's available
                        if (checkExtension(ext_c, availableInstanceExtensionNames))
                            { accum.get<0>().push_back(ext_c.data()); }
                    }
                }
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

                for (auto devExt_c : criteria_c.get_desiredDeviceExtensions())
                {
                    accum.get<5>().push_back(devExt_c);
                }

                for (auto feat_c : criteria_c.get_features())
                {
                    accum.get<6>().push_back(feat_c.first);
                }

                for (auto feat_c : criteria_c.get_desiredFeatures())
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
        bool _ = false;

        { // nested scope keeping me careful about not reusing ar
            AbilityResolver ar { aliases, abilities };
            for (auto inc_c : config_c.get_instanceInclude())
                { ar.include(inc_c); }

            if (auto const & criteria_c = config_c.get_sharedInstanceCriteria();
                criteria_c.has_value())
            {
                ok = ok && ar.doCrit(criteria_c->get_name(), * criteria_c, false, fn, accum, _, false);
            }
        }

        if (ok == false)
            { return false; }

        // TODO: Only those device groups we care about
        for (int devGroupIdx = 0; devGroupIdx < config_c.get_deviceProfileGroups().size(); ++devGroupIdx)
        {
            auto const & profileGroup_c = config_c.get_deviceProfileGroups()[devGroupIdx];
            auto mark = accum.mark();

            AbilityResolver ar { aliases, abilities };
            for (auto inc_c : profileGroup_c.get_include())
                { ar.include(inc_c); }

            auto & deviceInfo = deviceAssignments[devGroupIdx].expDeviceInfo;
            //deviceInfo = expInstInfo;

            auto devAccum = deviceInfo.makeAccumulator();

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

    void DeviceCreator::consolidateExploratoryCollections()
    {
        //... TODO: this
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

    void DeviceCreator::initExploratoryPhysDevices()
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
        AbilityResolver ar(aliases, abilities);

        int dpgIdx = getDeviceGroupIdx(deviceGroup);
        if (dpgIdx == -1)
            { return; }

        auto & devAss = deviceAssignments[dpgIdx];
        devAss.deviceSuitabilities.resize(physDevices.size());

        //  make structure chains based on exploratory device providers
        VkFeatures allInterestingFeatures;
        allInterestingFeatures.init(deviceAssignments[dpgIdx].expDeviceInfo.featureProviders);
        VkProperties allInterestingProperties;
        allInterestingProperties.init(deviceAssignments[dpgIdx].expDeviceInfo.propertyProviders);
        VkQueueFamilies allInterestingQfProperties;
        allInterestingQfProperties.init(deviceAssignments[dpgIdx].expDeviceInfo.queueFamilyPropertyProviders);

        for (int physIdx = 0; physIdx < physDevices.size(); ++physIdx)
        {
            auto & physDevice = physDevices[physIdx].physicalDevice;
            auto const & availableDeviceExts = physDevices[physIdx].availableDeviceExtensions;
            auto & suitability = devAss.deviceSuitabilities[physIdx];

            suitability.physicalDeviceIdx = physIdx;

            //  get features, props, etc from device
            VkFeatures availableFeatures = allInterestingFeatures.copyAndReset();
            VkProperties availableProperties = allInterestingProperties.copyAndReset();
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
                    VkQueueFamilies interestingQfProperties = allInterestingQfProperties.copyAndReset();
                    suitability.bestQueueVillageProfile = getBestQueueVillageProfile(dpgIdx, physIdx,
                        availableFeatures, availableProperties, interestingQfProperties);
                    if (suitability.bestQueueVillageProfile < 0)
                        { suitability.bestProfileIdx = NoGoodProfile; }
                }
            } while (suitability.bestProfileIdx < 0);

            //  if we found a profile:
            //      da.deviceProfiles[physDevIdx].bestProfileIdx = profileIdx
            //      da.deviceProfiles[physDevIdx].usedProviders = proUsed
        }
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
                        { accum.get<5>().push_back(devExt_c); }
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
                        { accum.get<5>().push_back(devExt_c); }
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
        };

        bool ok = true;
        bool _ = false;

        auto mark = accum.mark();

        //VkFeatures sharedFeatures = availbleFeatures.copyAndReset();

        { // nested scope keeping me careful about not reusing ar
            AbilityResolver ar { aliases, abilities };
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

        AbilityResolver ar { aliases, abilities };
        for (auto inc_c : profileGroup_c.get_include())
            { ar.include(inc_c); }

        //VkFeatures profileFeatures = availbleFeatures.copyAndReset();
        int profileIdx = ar.doProfileGroup(profileGroup_c.get_name(), profileGroup_c, false,
                                           fn, accum, _, false, startingProfileIdx);
        if (profileIdx != NoGoodProfile)
            { return profileIdx; }

        accum.rollBack(mark);
        return NoGoodProfile;
    }

    int DeviceCreator::getBestQueueVillageProfile(int devGroupIdx, int physDevIdx,
        VkFeatures const & availbleFeatures, VkProperties const & availableProperties,
        VkQueueFamilies const & availableQfProperties)
    {
        auto const & profileGroup_c = config_c.get_deviceProfileGroups()[devGroupIdx];
        auto & devAss = deviceAssignments[devGroupIdx];
        auto & devSuit = devAss.deviceSuitabilities[physDevIdx];
        auto & physDev = physDevices[physDevIdx];

        auto accum = devSuit.bestProfileDeviceInfo.makeAccumulator();

        int bestProfileIdx = devSuit.bestProfileIdx;

        // TODO: this

        // TODO: once we have a winning profile, let's add its criteria to the device creation accum
        // and its queueFamilyComposition to suitability

        return devSuit.bestQueueVillageProfile;
    }

    void DeviceCreator::assignDevices(std::string_view groupName, int numDevices)
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
            deviceAssignmentGroup.winningDeviceIdxs.push_back(bestDeviceIdx);
        }
    }

    /*
    void DeviceCreator::gatherFinalCreationSet(std::string_view deviceGroupName, int numDevices, VulkanSubsystem & vs)
    {
    }
    */

    void DeviceCreator::consolidateFinalCollections(VulkanSubsystem & vs)
    {
        vs.vulkanVersion = utilizedVulkanVersion;

        std::unordered_set<char const *> addedExts;
        std::unordered_set<char const *> addedLayers;
        std::unordered_set<std::string_view> addedDebugMessengers;
        std::unordered_set<VkValidationFeatureEnableEXT> addedEnabledValidations;
        std::unordered_set<VkValidationFeatureDisableEXT> addedDisabledValidations;

        for (int dai = 0; dai < deviceAssignments.size(); ++dai)
        {
            auto & ass = deviceAssignments[dai];
            for (int physDevIdx : ass.winningDeviceIdxs)
            {
                std::unordered_set<char const *> addedDevExts;
                std::unordered_map<std::string_view, 
                                   std::unordered_set<std::string_view>> addedFeatures;
                std::unordered_set<std::string_view> addedProperties;
                std::unordered_set<std::string_view> addedQueueFamilyProperties;

                auto & suit = ass.deviceSuitabilities[physDevIdx];
                auto & idi = suit.bestProfileDeviceInfo;

                vs.devices.push_back(DeviceSubsystem {});
                DeviceSubsystem & ds = vs.devices[vs.devices.size() - 1];

                ds.physicalDeviceIdx = physDevIdx;
                ds.deviceGroupIdx = dai;
                ds.deviceGroupName = config_c.get_deviceProfileGroups()[dai].get_name();
                
                std::vector<std::string_view> fps;
                for (auto const & [fp, _] : ass.expDeviceInfo.featureProviders)
                    { fps.push_back(fp); }
                ds.features.init(fps);

                for (auto val : idi.extensions)
                {
                    if (auto [_, didInsert] = addedExts.insert(val); didInsert)
                        { vs.extensions.push_back(val); }
                }
                for (auto val : idi.layers)
                {
                    if (auto [_, didInsert] = addedLayers.insert(val); didInsert)
                        { vs.layers.push_back(val); }
                }
                for (auto const & val : idi.debugMessengers)
                {
                    if (auto [_, didInsert] = addedDebugMessengers.insert(std::get<0>(val)); didInsert)
                        { vs.debugMessengers.push_back(std::get<1>(val)); }
                }
                for (auto const & val : idi.enabledValidations)
                {
                    if (auto [_, didInsert] = addedEnabledValidations.insert(val); didInsert)
                        { vs.enabledValidations.push_back(val); }
                }
                for (auto const & val : idi.disabledValidations)
                {
                    if (auto [_, didInsert] = addedDisabledValidations.insert(val); didInsert)
                        { vs.disabledValidations.push_back(val); }
                }
                for (auto val : idi.deviceExtensions)
                {
                    if (auto [_, didInsert] = addedDevExts.insert(val); didInsert)
                        { ds.deviceExtensions.push_back(val); }
                }
                for (auto const & [provider, feature] : idi.featureProviders)
                {
                    std::unordered_set<std::string_view> newSet {};
                    auto [itprov, _] = addedFeatures.insert(std::make_pair(provider, newSet));
                    auto [_, didInsert] = itprov->second.insert(feature);
                    if (didInsert)
                        { ds.features.set(provider, feature, VK_TRUE); }
                }
            }
        }

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
    }

    void DeviceCreator::makeFinalInstance(VulkanSubsystem & vs)
    {
        // TODO NEXT:  vs.debugMessengers stuff instead of expInstInfo
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

    void DeviceCreator::makeDevices(VulkanSubsystem & vs)
    {

//                    turn accumulated features into VkFeatures with set() calls

    }

    void DeviceCreator::makeQueues(VulkanSubsystem & vs)
    {

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
                    suitability.queueFamilyComposition = std::move(queueFamilyAlloc);
                }
            }
        }
        deviceAssignmentGroup.hasBeenComputed = true;
    }


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

}
