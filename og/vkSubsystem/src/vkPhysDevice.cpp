#include "../inc/deviceCreator.hpp"
#include "../../app/inc/app.hpp"
#include "../../logger/inc/logger.hpp"
#include "../../gen/inc/og.hpp"
#include <map>
#include <algorithm>
#include <numeric>

using namespace std::literals::string_view_literals;

namespace og
{
    PhysVkDevice::PhysVkDevice()
    // : deviceCreator(deviceCreator)
    {
    }

    void PhysVkDevice::init(int physIdx, VkPhysicalDevice phdev)
    {
        physicalDeviceIdx = physIdx;
        physicalDevice = phdev;

        log(fmt::format("Physical device {}:", physIdx));

        std::vector<VkExtensionProperties> availableDeviceExtensionsVect;

        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(phdev, nullptr, & count, nullptr);
        availableDeviceExtensionsVect.resize(count);
        vkEnumerateDeviceExtensionProperties(phdev, nullptr, & count, availableDeviceExtensionsVect.data());
        for (int i = 0; i < count; ++i)
        {
            availableDeviceExtensions.insert(
                availableDeviceExtensionsVect[i].extensionName);
        }

        for (auto const & elem : availableDeviceExtensionsVect)
            { log(fmt::format(". available device extension: {} v{}", elem.extensionName, elem.specVersion)); }

            
    }

    /*
    // Check profiles one by one until the device can meet the profile's requirements.
    // That's the best profile the device can do; the device with the best profile idx
    // will win.
    int PhysVkDevice::findBestProfileIdx(int groupIdx, vkSubsystem::physDeviceProfileGroup const & group_c, PhysicalDeviceSuitability & suitability)
    {
        bool reportAll = true;

        if (availableQueueTypes == 0)
        {

            // This is adjacent to the QF properties stored in a PhysicalDeviceSuitability.
            // That looks for chain structures; this only needs to consider queue types, so
            // we just get the basic structs.
            uint32_t devQueueFamilyCount = 0;
            std::vector<VkQueueFamilyProperties> availableQueueFamilies;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, & devQueueFamilyCount, nullptr);
            availableQueueFamilies.resize(devQueueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, & devQueueFamilyCount, availableQueueFamilies.data());
            for (auto const & qfp : availableQueueFamilies)
                { availableQueueTypes = static_cast<VkQueueFlagBits>(static_cast<int>(availableQueueTypes) | static_cast<int>(qfp.queueFlags)); }
        }

        auto const & groupName_c = group_c.get_name();
        auto const & profiles_c = group_c.get_profiles();
        int selectedProfileIdx = -1;
        for (int profileIdx = 0; profileIdx < profiles_c.size(); ++profileIdx)
        {
            auto const & profile_c = profiles_c[profileIdx];
            if (profile_c.get_enabled() == false)
                { continue; }

            log(fmt::format(". . . Scoring profile {}:", profile_c.get_name()));

            bool noGood = false;

            auto & profileSpecificCriteria = suitability.profileCritera[profileIdx];

            // Get all required and desired providers. This lets us build the structure chains
            // we care about for querying (and later, creation if this is the device provile
            // we wind up using).
            std::vector<std::string_view> featureProviders;
            std::vector<std::string_view> propertyProviders;
            auto getFeaturesAndProperties = [& featureProviders, & propertyProviders](auto const & criteria_c)
            {
                auto const & features_c = criteria_c.get_features();
                for (auto const & [key, _] : features_c)
                    { featureProviders.push_back(key); }
                auto const & desFeatures_c = criteria_c.get_desiredFeatures();
                for (auto const & [key, _] : desFeatures_c)
                    { featureProviders.push_back(key); }
                auto const & properties_c = criteria_c.get_properties();
                for (auto const & [key, _] : properties_c)
                    { propertyProviders.push_back(key); }
            };

            if (group_c.get_sharedCriteria().has_value())
                { getFeaturesAndProperties(* group_c.get_sharedCriteria()); }
            getFeaturesAndProperties(profile_c);

            profileSpecificCriteria.features.init(featureProviders);
            profileSpecificCriteria.properties.init(propertyProviders);

            // now actually fill in the structures from the physical device
            vkGetPhysicalDeviceFeatures2(physicalDevice, & profileSpecificCriteria.features.mainStruct);
            vkGetPhysicalDeviceProperties2(physicalDevice, & profileSpecificCriteria.properties.mainStruct);

            // now check the results
            auto check = [this, & noGood, & reportAll](auto const & criteria_c)
            {
                // NOTE: these will check against SELECTED extensions, layers, etc
                // from instance creation.
                auto const & vulkanVersion_c =  criteria_c.get_vulkanVersion();
                if (vulkanVersion_c.has_value())
                {
                    if (deviceCreator.checkVulkan(* vulkanVersion_c) == false)
                        { noGood = true; log(". . . . Vulkan version '{}' requirement not met."); if (! reportAll) { return; } }
                }

                auto const & extensions_c = criteria_c.get_extensions();
                for (auto const & extension_c : extensions_c)
                {
                    if (deviceCreator.checkExtension(extension_c) == false)
                        { noGood = true; log(fmt::format(". . . . Extension '{}' requirement not met.", extension_c)); if (! reportAll) { return; } }
                }

                auto const & layers_c = criteria_c.get_layers();
                for (auto const & layer_c : layers_c)
                {
                    if (deviceCreator.checkLayer(layer_c) == false)
                        { noGood = true; log(fmt::format(". . . . Layer '{}' requirement not met.", layer_c)); if (! reportAll) { return; } }
                }
            };

            if (group_c.get_sharedCriteria().has_value())
                { check(* group_c.get_sharedCriteria()); }
            check(profile_c);

            // These will check against AVAILABLE device extensions, queueTypes, and features.

            auto const & deviceExtensions_c = profile_c.get_deviceExtensions();
            for (auto const & deviceExtension_c : deviceExtensions_c)
            {
                if (checkDeviceExtension(deviceExtension_c) == false)
                    { noGood = true; log(fmt::format(". . . . Device extension '{}' requirement not met.", deviceExtension_c)); if (! reportAll) { break; } }
            }

            auto const & queueTypesInc_c = profile_c.get_queueTypesIncluded();
            auto const & queueTypesExc_c = profile_c.get_queueTypesExcluded();
            if (checkQueueTypes(
                queueTypesInc_c.has_value() ? * queueTypesInc_c : static_cast<VkQueueFlagBits>(0),
                queueTypesExc_c.has_value() ? * queueTypesExc_c : static_cast<VkQueueFlagBits>(0))
                == false)
                { noGood = true; log(". . . . Queue types requirement not met."); if (! reportAll) { break; } }

            auto const & featuresMap_c = profile_c.get_features();
            for (auto const & [provider_c, features_c] : featuresMap_c)
            {
                for (auto const & feature_c : features_c)
                {
                    try
                    {
                        if (profileSpecificCriteria.features.check(provider_c, feature_c)
                            == false)
                            { noGood = true; log(fmt::format(". . . . requirement not met: features / {} / {}", provider_c, feature_c)); if (! reportAll) { break; } }
                    }
                    catch (std::runtime_error & e)
                        { noGood = true; log(fmt::format(". . . . requirement error: features / {} / {}: {}", provider_c, feature_c, e.what())); if (! reportAll) { break; } }
                }
            }


            auto const & propertiesMap_c = profile_c.get_properties();
            for (auto const & [provider_c, properties_c] : propertiesMap_c)
            {
                for (auto const & [property_c, op_c, value_c] : properties_c)
                {
                    try
                    {
                        if (profileSpecificCriteria.properties.check(provider_c, property_c, op_c, value_c)
                            == false)
                            { noGood = true; log(fmt::format(". . . . requirement not met: properties / {} / {}", provider_c, property_c)); if (! reportAll) { break; } }
                    }
                    catch (std::runtime_error & e)
                        { noGood = true; log(fmt::format(". . . . requirement error: properties / {} / {}: {}", provider_c, property_c, e.what())); if (! reportAll) { break; } }
                }
            }

            // We don't check desired features or desired extensions. We'll just add
            // them later if we can.

            if (noGood == false)
            {
                selectedProfileIdx = profileIdx;

                log(fmt::format(". . . Best suitable profile found: {} (profile #{})",
                    profile_c.get_name(), selectedProfileIdx));
                break;
            }
        }

        if (selectedProfileIdx == -1)
        {
            log(fmt::format(". Could not find a physical device for profile group '{}'.", groupName_c));
            // Not necessarily an error; a given profile group may just be a nice-to-have (require 0 devices).
            return -1;
        }

        return selectedProfileIdx;
    }
    */

   /*
    std::tuple<int, QueueFamilyComposition>
        PhysVkDevice::findBestQueueFamilyAllocation(int groupIdx,
            vkSubsystem::physDeviceProfileGroup const & group_c,
            int profileIdx)
    {
        bool reportAll = true;

        log(fmt::format(". . . findBestQueueFamilyAllocation(group={}, profile={})", groupIdx, profileIdx));

        // Find the best matching queue family group.
        auto const & qvProfiles_c = group_c.get_queueVillageProfiles();
        auto & deviceAssignmentGroup = deviceCreator.deviceAssignments[groupIdx];
        auto & suitability = deviceAssignmentGroup.deviceSuitabilities[physicalDeviceIdx];
        auto & profileSpecificCriteria = suitability.profileCritera[profileIdx];
        auto & suitabilityQfs = suitability.queueFamilies;
        for (uint32_t qvProfileIdx_c = 0; qvProfileIdx_c < qvProfiles_c.size(); ++qvProfileIdx_c)
        {
            auto const & qvProfile_c = qvProfiles_c[qvProfileIdx_c];

            log(fmt::format(". . . . checking qfprofile {} (index #{})", qvProfile_c.get_name(), qvProfileIdx_c));

            auto const & qvillage_c = qvProfile_c.get_queueVillage();
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

                auto const & qfAttribs_c = qvillage_c[qfaIdx_c];
                // starts empty; will collect each qfi that meets the requirements
                auto & selectable = selectableQueueFamilyIndices[qfaIdx_c];
                for (int devQfi = 0; devQfi < suitabilityQfs.size(); ++devQfi)
                {
                    auto const & devQueueFamily = suitabilityQfs[devQfi];
                    bool noGood = false;

                    if (qfAttribs_c.get_criteria().has_value())
                    {
                        auto const & featuresMap_c = qfAttribs_c.get_criteria()->get_features();
                        for (auto const & [provider_c, features_c] : featuresMap_c)
                        {
                            for (auto const & feature_c : features_c)
                            {
                                try
                                {
                                    if (profileSpecificCriteria.features.check(provider_c, feature_c)
                                        == false)
                                        { noGood = true; log(fmt::format(". . . . . . requirement not met: features / {} / {}", provider_c, feature_c)); if (! reportAll) { break; } }
                                }
                                catch (std::runtime_error & e)
                                    { noGood = true; log(fmt::format(". . . . . . requirement error: features / {} / {}: {}", provider_c, feature_c, e.what())); if (! reportAll) { break; } }
                            }
                        }

                        auto const & propertiesMap_c = qfAttribs_c.get_criteria()->get_properties();
                        for (auto const & [provider_c, properties_c] : propertiesMap_c)
                        {
                            for (auto const & [property_c, op_c, value_c] : properties_c)
                            {
                                try
                                {
                                    if (profileSpecificCriteria.properties.check(provider_c, property_c, op_c, value_c)
                                        == false)
                                        { noGood = true; log(fmt::format(". . . . . . requirement not met: properties / {} / {}", provider_c, property_c)); if (! reportAll) { break; } }
                                }
                                catch (std::runtime_error & e)
                                    { noGood = true; log(fmt::format(". . . . . . requirement error: properties / {} / {}: {}", provider_c, property_c, e.what())); if (! reportAll) { break; } }
                            }
                        }

                        auto const & qfPropertiesMap_c = qfAttribs_c.get_criteria()->get_queueFamilyProperties();
                        for (auto const & [provider_c, properties_c] : propertiesMap_c)
                        {
                            for (auto const & [property_c, op_c, value_c] : properties_c)
                            {
                                try
                                {
                                    if (suitability.queueFamilies[devQfi].check(provider_c, property_c, op_c, value_c)
                                        == false)
                                        { noGood = true; log(fmt::format(". . . . . . requirement not met: queue family properties / {} / {}", provider_c, property_c)); if (! reportAll) { break; } }
                                }
                                catch (std::runtime_error & e)
                                    { noGood = true; log(fmt::format(". . . . . . requirement error: queue family properties / {} / {}: {}", provider_c, property_c, e.what())); if (! reportAll) { break; } }
                            }
                        }
                    }

                    if (noGood == false)
                    {
                        log(fmt::format(". . . . . . qf index {} is selectable", devQfi));
                        selectable.push_back(devQfi);
                    }
                    else
                    {
                        log(fmt::format(". . . . . . qf index {} is not selectable", devQfi));
                    }
                }

                if (selectable.size() == 0)
                    { qfProfileFail = true; }
            }

            if (qfProfileFail)
            {
                log(fmt::format(". . . . . Queue family profile {} - no match.", qvProfileIdx_c));
                continue;
            }

            auto numCombos = std::accumulate(begin(selectableQueueFamilyIndices), end(selectableQueueFamilyIndices),
                1, [](uint32_t b, auto & sub){ return static_cast<uint32_t>(sub.size()) * b; });
            auto comboScores = std::vector<uint32_t>(numCombos, 0);
            uint32_t winningScore = 0;
            uint32_t winningCombo = -1;

            // Now find the best unique qfi assignment for each queue.
            // If there is not one, go to next qfprofile.

            // This monsta lets us recurse to n dimensions to score up a village.
            auto fn = [&](uint32_t famIdx, std::bitset<64> busyFamilies, uint32_t & comboIdx, uint64_t score) -> void
            {
                auto fn_int = [&](uint32_t famIdx, std::bitset<64> busyFamilies, uint32_t & comboIdx, uint64_t score, auto outer_fn) -> void
                {
                    auto const & qfis = selectableQueueFamilyIndices[famIdx];
                    for (auto const & qfi : qfis)
                    {
                        if (busyFamilies[qfi])
                        {
                            score = 0;
                        }
                        else
                        {
                            auto numQueuesOnDevice = suitabilityQfs[qfi].mainStruct.queueFamilyProperties.queueCount;
                            auto numQueuesDesired = qvProfile_c.get_queueVillage()[famIdx].get_maxQueueCount();
                            score *= std::min(numQueuesOnDevice, numQueuesDesired);
                        }
                        busyFamilies[qfi] = true;

                        if (famIdx + 1 == selectableQueueFamilyIndices.size())
                        {
                            comboScores[comboIdx] = score;
                            if (score > winningScore)
                            {
                                winningScore = score;
                                winningCombo = comboIdx;
                                log(fmt::format("@ leading combo: {} score {}", winningCombo, winningScore));
                            }
                            comboIdx += 1;
                        }
                        else
                        {
                            outer_fn(famIdx + 1, busyFamilies & std::bitset<64> { 1ULL << qfi }, comboIdx, score, outer_fn);
                        }
                    }
                };

                fn_int(famIdx, busyFamilies, comboIdx, score, fn_int);
            };

            uint32_t comboIdx = 0;
            fn(0, 0, comboIdx, 1);

            QueueFamilyComposition qfiAllocs;
            qfiAllocs.queueFamilies.resize(qvProfile_c.get_queueVillage().size());

            if (winningCombo != -1)
            {
                log(fmt::format("@ winning combo: {}", winningCombo));

                bool done = false;
                comboIdx = 0;
                auto fn2 = [&](uint32_t famIdx, uint32_t & comboIdx) -> void
                {
                    auto fn2_int = [&](uint32_t famIdx, uint32_t & comboIdx, auto outer_fn2) -> void
                    {
                        auto const & qfis = selectableQueueFamilyIndices[famIdx];
                        for (auto const & qfi : qfis)
                        {
                            if (done)
                                { return; }

                            qfiAllocs.queueFamilies[famIdx] = { qfi };

                            if (famIdx + 1 == selectableQueueFamilyIndices.size())
                            {
                                if (comboIdx == winningCombo)
                                {
                                    done = true;
                                    return;
                                }

                                comboIdx += 1;
                            }
                            else
                            {
                                outer_fn2(famIdx + 1, comboIdx, outer_fn2);
                            }
                        }
                    };

                    fn2_int(famIdx, comboIdx, fn2_int);
                };

                for (int allocIdx = 0; allocIdx < qfiAllocs.queueFamilies.size(); ++allocIdx)
                {
                    auto & alloc = qfiAllocs.queueFamilies[allocIdx];
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

                // we found a winner, recorded the qfi data, now bail
                log(fmt::format(". Best suitable queue family profile found: {} (profile #{})",
                    qvProfiles_c[qvProfileIdx_c].get_name(),
                    qvProfileIdx_c));

                return {qvProfileIdx_c, qfiAllocs};
            }

            // no winner; we move to the next village profile
        }

        log(fmt::format(". Could not find a suitable queue family combination for profile group '{}'.", group_c.get_name()));
        // Not necessarily an error; a given profile group may just be a nice-to-have (require 0 devices).
        return {-1, {}};
    }


    void PhysVkDevice::createVkDevice()
    {
        if (groupIdx == -1)
        {
            log(fmt::format("Device {} is not suitable for any profile group.", physicalDeviceIdx));
            return;
        }

        auto const & group_c = deviceCreator.config_c.get_deviceProfileGroups()[groupIdx];
        auto const & profile_c = group_c.get_profiles()[profileIdx];

        std::vector<char const *> requiredDeviceExtensions;
        std::vector<char const *> requiredLayers;

        auto & assignment = deviceCreator.deviceAssignments[groupIdx];
        auto & suitability = assignment.deviceSuitabilities[physicalDeviceIdx];
        auto const & profileCriteria = suitability.profileCritera[suitability.bestProfileIdx];
        createdCapabilities.features = profileCriteria.features.copyAndReset();

        auto requireExtsAndLayersEtc = [&](auto const & criteria_c)
        {
            // remaking the layers from instance creation
            for (auto const & layerName_c : criteria_c.get_layers())
            {
                auto it = std::find_if(begin(deviceCreator.get_utilizedLayers()), end(deviceCreator.get_utilizedLayers()),
                    [& layerName_c](auto && ae){ return layerName_c == ae; } );
                if (it != end(deviceCreator.get_utilizedLayers()))
                    { requiredLayers.push_back(it->data()); }
            }
            for (auto const & deviceExtension_c : criteria_c.get_deviceExtensions())
            {
                auto it = std::find_if(begin(availableDeviceExtensions), end(availableDeviceExtensions),
                    [& deviceExtension_c](auto && ae){ return deviceExtension_c == ae.extensionName; } );
                if (it != end(availableDeviceExtensions))
                {
                    createdCapabilities.deviceExtensions.push_back(it->extensionName);
                    requiredDeviceExtensions.push_back(it->extensionName);
                }
            }
            for (auto const & deviceExtension_c : criteria_c.get_desiredDeviceExtensions())
            {
                auto it = std::find_if(begin(availableDeviceExtensions), end(availableDeviceExtensions),
                    [& deviceExtension_c](auto && ae){ return deviceExtension_c == ae.extensionName; } );
                if (it != end(availableDeviceExtensions))
                {
                    createdCapabilities.deviceExtensions.push_back(it->extensionName);
                    requiredDeviceExtensions.push_back(it->extensionName);
                }
            }
            for (auto const & [provider_c, features_c] : criteria_c.get_features())
            {
                for (auto const & feature_c : features_c)
                {
                    if (profileCriteria.features.check(provider_c, feature_c))
                        { createdCapabilities.features.set(provider_c, feature_c, VK_TRUE); }
                }
            }
            for (auto const & [provider_c, features_c] : criteria_c.get_desiredFeatures())
            {
                for (auto const & feature_c : features_c)
                {
                    if (profileCriteria.features.check(provider_c, feature_c))
                        { createdCapabilities.features.set(provider_c, feature_c, VK_TRUE); }
                }
            }
        };

        if (group_c.get_sharedCriteria().has_value())
            { requireExtsAndLayersEtc(* group_c.get_sharedCriteria()); }
        requireExtsAndLayersEtc(profile_c);

        createdCapabilities.queueFamilies = std::move(suitability.queueFamilies);
        suitability.queueFamilies.clear();
        createdCapabilities.queueFamilyComposition = std::move(suitability.queueFamilyAssignments);
        suitability.queueFamilyAssignments = {};

        log(". using device extensions: ");
        for (auto & re : requiredDeviceExtensions)
            { log(fmt::format("  {}", re)); }

        log(". using queue families: ");
        for (auto const & queueFam : createdCapabilities.queueFamilyComposition.queueFamilies)
        {
            log(fmt::format(". . queue family index: {}\n     . . count: {}\n     . . flags: {}",
                queueFam.qfi, queueFam.count, static_cast<uint32_t>(queueFam.flags)));
        }

        auto dqcis = std::vector<VkDeviceQueueCreateInfo>(createdCapabilities.queueFamilyComposition.queueFamilies.size());
        for (int i = 0; i < createdCapabilities.queueFamilyComposition.queueFamilies.size(); ++i)
        {
            auto const & [qfi, count, flags, priorities, globalPriority] = createdCapabilities.queueFamilyComposition.queueFamilies[i];

            void * pNext = NULL;
            VkDeviceQueueGlobalPriorityCreateInfoKHR globalPriorityStruct = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_KHR, NULL, VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR };
            if (globalPriority.has_value())
            {
                pNext = & globalPriorityStruct;
                globalPriorityStruct.globalPriority = * globalPriority;
            }

            dqcis[i] =
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = pNext,
                .flags = flags,
                .queueFamilyIndex = qfi,
                .queueCount = count,
                .pQueuePriorities = priorities.data()
            };
        }

        VkDeviceCreateInfo dci =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = & createdCapabilities.features.mainStruct,
            .flags = 0,
            .queueCreateInfoCount = static_cast<uint32_t>(dqcis.size()),
            .pQueueCreateInfos = dqcis.data(),
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = requiredDeviceExtensions.data(),
            .pEnabledFeatures = NULL
        };

        VKR(vkCreateDevice(physicalDevice, & dci, nullptr, & device));
        log("IT WORKED");
    }

    void PhysVkDevice::destroyVkDevice()
    {
        if (device != nullptr)
        {
            vkDestroyDevice(device, nullptr);
        }
    }
    */

    /*
    bool PhysVkDevice::checkDeviceExtension(std::string_view deviceExtension)
    {
        // If we haven't made our device yet, check against available device extensions.
        // Otherwise, check against what we declared to make the device.
        if (device == nullptr)
        {
            auto aeit = std::find_if(begin(availableDeviceExtensions), end(availableDeviceExtensions),
                [& deviceExtension](auto && elem) { return elem.extensionName == deviceExtension; });
            return aeit != end(availableDeviceExtensions);
        }
        else
        {
            auto aeit = std::find_if(begin(createdCapabilities.deviceExtensions), end(createdCapabilities.deviceExtensions),
                [& deviceExtension](auto && elem) { return elem == deviceExtension; });
            return aeit != end(createdCapabilities.deviceExtensions);
        }
    }

    bool PhysVkDevice::checkQueueTypes(VkQueueFlagBits queueTypesIncluded, VkQueueFlagBits queueTypesExcluded)
    {
        // If we haven't made our device yet, check against available device extensions.
        // Otherwise, check against what we declared to make the device.
        using uenum_t = std::underlying_type_t<VkQueueFlagBits>;
        VkQueueFlagBits queueTypesAvailable = static_cast<VkQueueFlagBits>(0);
        if (device == nullptr)
        {
            queueTypesAvailable = static_cast<VkQueueFlagBits>(
                static_cast<uenum_t>(queueTypesAvailable) |
                static_cast<uenum_t>(availableQueueTypes));
        }
        else
        {
            for (auto const & qfProps : createdCapabilities.queueFamilyComposition.queueFamilies)
            {
                queueTypesAvailable = static_cast<VkQueueFlagBits>(
                    static_cast<uenum_t>(queueTypesAvailable) |
                    static_cast<uenum_t>(createdCapabilities.queueFamilies[qfProps.qfi].mainStruct.queueFamilyProperties.queueFlags));
            }
        }

        return (queueTypesAvailable & queueTypesIncluded) == queueTypesIncluded &&
               (queueTypesAvailable & queueTypesExcluded) == 0;
    }
    */
}

/*
subgroup
group
memory_2
point_clipping
multiview
protected_memory
id
maintenance_3
vulkan_1_1
vulkan_1_2
driver
float_controls
descriptor_indexing
depth_stencil_resolve
sampler_filter_minmax
timeline_semaphore
vulkan_1_3
tool
subgroup_size_control
inline_uniform_block
shader_integer_dot_product
texel_buffer_alignment
maintenance_4
transform_feedback_ext
push_descriptor_khr
multiview_per_view_attributes_nvx
discard_rectangle_ext
conservative_rasterization_ext
performance_query_khr
sample_locations_ext
blend_operation_advanced_ext
acceleration_structure_khr
ray_tracing_pipeline_khr
shader_sm_builtins_nv
shading_rate_image_nv
ray_tracing_nv
external_memory_host_ext
shader_core_amd
vertex_attribute_divisor_ext
mesh_shader_nv
pci_bus_info_ext
fragment_density_map_ext
fragment_shading_rate_khr
shader_core_2_amd
memory_budget_ext
cooperative_matrix_nv
provoking_vertex_ext
line_rasterization_ext
pipeline_executable_features_khr
device_generated_commands_nv
robustness_2_ext
custom_border_color_ext
graphics_pipeline_library_ext
fragment_shading_rate_enums_nv
fragment_density_map_2_ext
drm_ext
subpass_shading_huawei
multi_draw_ext
fragment_density_map_offset_qcom
multiview_properties_khr
*/