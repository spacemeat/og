#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../../gen/inc/og.hpp"
#include <map>
#include <algorithm>
#include <numeric>
#include "../inc/utils.hpp"

using namespace std::literals::string_view_literals;

namespace og
{
    PhysVkDevice::PhysVkDevice()
    {
    }

    void Engine::initPhysVkDevices()
    {
        auto const & workUnits = appConfig.get_works();

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

        auto const & profileGroups_c = config.get_vkDeviceProfileGroups();
        deviceAssignments.resize(profileGroups_c.size());
    }

    void PhysVkDevice::init(int physIdx, VkPhysicalDevice phdev)
    {
        physicalDeviceIdx = physIdx;
        physicalDevice = phdev;

        log(fmt::format("Physical device {}:", physIdx));

        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(phdev, nullptr, & count, nullptr);
        std::vector<VkExtensionProperties> & availableDes = availableDeviceExtensions;
        availableDes.resize(count);
        vkEnumerateDeviceExtensionProperties(phdev, nullptr, & count, availableDes.data());

        for (auto const & elem : availableDes)
            { log(fmt::format(". available device extension: {} v{}", elem.extensionName, elem.specVersion)); }
    }

    void Engine::computeBestProfileGroupDevices(int groupIdx)
    {
        auto const & profileGroups_c = config.get_vkDeviceProfileGroups();
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

                uint32_t devQueueFamilyCount = device.availableQueueFamilies.size();
                vkGetPhysicalDeviceQueueFamilyProperties2(device.physicalDevice, & devQueueFamilyCount, nullptr);
                std::vector<VkQueueFamilyProperties2> qfPropsVect(devQueueFamilyCount);
                suitability.queueFamilies.resize(devQueueFamilyCount);
                for (int i = 0; i < devQueueFamilyCount; ++i)
                {
                    suitability.queueFamilies[i].init(queueFamilyPropertyProviders);
                    qfPropsVect[i] = suitability.queueFamilies[i].mainStruct;
                }
                vkGetPhysicalDeviceQueueFamilyProperties2(device.physicalDevice, & devQueueFamilyCount, device.availableQueueFamilies.data());
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

    // Check profiles one by one until the device can meet the profile's requirements.
    // That's the best profile the device can do; the device with the best profile idx
    // will win.
    int PhysVkDevice::findBestProfileIdx(int groupIdx, engine::physicalVkDeviceProfileGroup const & group_c, PhysicalDeviceSuitability & suitability)
    {
        bool reportAll = true;

        if (availableQueueFamilies.size() == 0)
        {
            // This is adjacent to the QF properties stored in a PhysicalDeviceSuitability.
            // That looks for chain structures; this only needs to consider queue types, so
            // we just get the basic structs.
            uint32_t devQueueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, & devQueueFamilyCount, nullptr);
            availableQueueFamilies.resize(devQueueFamilyCount);
            for (int i = 0; i < devQueueFamilyCount; ++i)
                { availableQueueFamilies[i] = { VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2 }; }
            vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, & devQueueFamilyCount, availableQueueFamilies.data());
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
                if (criteria_c.has_value())
                {
                    auto const & features_c = criteria_c->get_features();
                    for (auto const & [key, _] : features_c)
                        { featureProviders.push_back(key); }
                    auto const & properties_c = criteria_c->get_properties();
                    for (auto const & [key, _] : properties_c)
                        { propertyProviders.push_back(key); }
                }
            };

            getFeaturesAndProperties(group_c.get_requires());
            getFeaturesAndProperties(group_c.get_desires());
            getFeaturesAndProperties(profile_c.get_requires());
            getFeaturesAndProperties(profile_c.get_desires());

            profileSpecificCriteria.features.init(featureProviders);
            profileSpecificCriteria.properties.init(propertyProviders);

            // now actually fill in the structures from the physical device
            vkGetPhysicalDeviceFeatures2(physicalDevice, & profileSpecificCriteria.features.mainStruct);
            vkGetPhysicalDeviceProperties2(physicalDevice, & profileSpecificCriteria.properties.mainStruct);

            // now check the results
            auto check = [& noGood, & reportAll](auto const & criteria_c)
            {
                // NOTE: these will check against SELECTED extensions, layers, etc
                // from instance creation.
                if (criteria_c.has_value())
                {
                    auto const & vulkanVersion_c =  criteria_c->get_vulkanVersion();
                    if (vulkanVersion_c.has_value())
                    {
                        if (e->checkVulkan(* vulkanVersion_c) == false)
                            { noGood = true; log(". . . . Vulkan version '{}' requirement not met."); if (! reportAll) { return; } }
                    }

                    auto const & extensions_c = criteria_c->get_extensions();
                    for (auto const & extension_c : extensions_c)
                    {
                        if (e->checkExtension(extension_c) == false)
                            { noGood = true; log(fmt::format(". . . . Extension '{}' requirement not met.", extension_c)); if (! reportAll) { return; } }
                    }

                    auto const & layers_c = criteria_c->get_layers();
                    for (auto const & layer_c : layers_c)
                    {
                        if (e->checkLayer(layer_c) == false)
                            { noGood = true; log(fmt::format(". . . . Layer '{}' requirement not met.", layer_c)); if (! reportAll) { return; } }
                    }
                }
            };

            check(group_c.get_requires());
            check(profile_c.get_requires());

            // These will check against AVAILABLE device extensions, queueTypes, and features.

            auto const & deviceExtensions_c = profile_c.get_requires()->get_deviceExtensions();
            for (auto const & deviceExtension_c : deviceExtensions_c)
            {
                if (checkDeviceExtension(deviceExtension_c) == false)
                    { noGood = true; log(fmt::format(". . . . Device extension '{}' requirement not met.", deviceExtension_c)); if (! reportAll) { break; } }
            }

            auto const & queueTypesInc_c = profile_c.get_requires()->get_queueTypesIncluded();
            auto const & queueTypesExc_c = profile_c.get_requires()->get_queueTypesExcluded();
            if (checkQueueTypes(
                queueTypesInc_c.has_value() ? * queueTypesInc_c : static_cast<VkQueueFlagBits>(0),
                queueTypesExc_c.has_value() ? * queueTypesExc_c : static_cast<VkQueueFlagBits>(0))
                == false)
                { noGood = true; log(". . . . Queue types requirement not met."); if (! reportAll) { break; } }


            auto const & featuresMap_c = profile_c.get_requires()->get_features();
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


            auto const & propertiesMap_c = profile_c.get_requires()->get_properties();
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

    std::tuple<int, QueueFamilyComposition>
        PhysVkDevice::findBestQueueFamilyAllocation(int groupIdx,
            engine::physicalVkDeviceProfileGroup const & group_c,
            int profileIdx)
    {
        bool reportAll = true;

        log(fmt::format(". . . findBestQueueFamilyAllocation(group={}, profile={})", groupIdx, profileIdx));

        // Find the best matching queue family group.
        uint32_t selectedQueueFamilyProfileIdx = -1;
        QueueFamilyComposition qfiAllocs;
        auto const & qvProfiles_c = group_c.get_queueVillageProfiles();
        auto & deviceAssignmentGroup = e->deviceAssignments[groupIdx];
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

                    if (qfAttribs_c.get_requires().has_value())
                    {
                        auto const & featuresMap_c = qfAttribs_c.get_requires()->get_features();
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

                        auto const & propertiesMap_c = qfAttribs_c.get_requires()->get_properties();
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

                        auto const & qfPropertiesMap_c = qfAttribs_c.get_requires()->get_queueFamilyProperties();
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

            // Now find the best unique qfi assignment for each queue.
            // If there is not one, go to next qfprofile.



            // Check this by building a vector of all combinations of qfi indices in selectableQueueFamilyIndices.
            // Some of these combinations will be invalid--for instance, an index can only be used once, so any
            // combination that has repeated indices will be an invalid combination. We store in the vector the
            // product of min(queues, desiredQueues) for each qfi. The combination with the highest desired queue
            // count is the most ideal selections of queue family indices.

            auto numCombos = std::accumulate(begin(selectableQueueFamilyIndices), end(selectableQueueFamilyIndices),
                1, [](uint32_t b, auto & sub){ return static_cast<uint32_t>(sub.size()) * b; });
            auto comboScores = std::vector<uint32_t>(numCombos, 0);

            uint32_t numQs = selectableQueueFamilyIndices.size();
            std::vector<uint32_t> results(numQs * numCombos);
            for (int q = 0; q < numQs; ++q)
            {
                // for each q, determine the vertical repetition = the product of the remaining q counts
                int vertStride = std::accumulate(next(begin(selectableQueueFamilyIndices), q), end(selectableQueueFamilyIndices),
                    1, [](uint32_t b, auto & sub){ return static_cast<uint32_t>(sub.size()) * b; });
                int numSel = selectableQueueFamilyIndices[q].size();
                int cycles = numCombos / vertStride / numSel;

                for (int cycle = 0; cycle < cycles; ++cycle)
                {
                    for (int selection = 0; selection < numSel; ++selection)
                    {
                        int val = selectableQueueFamilyIndices[q][selection];
                        for (int i = 0; i < vertStride; ++i)
                        {
                            int idx = cycle * numSel * vertStride * numQs
                                            + selection * vertStride * numQs
                                                        + i * numQs
                                                            + q;
                            results[idx] = val;
                        }
                    }
                }
            }

            // Now score each combo. Any with repeated qfis scores 0. The rest are scored by
            // how well they match the desired queue counts, all multiplied together.
            int bestScore = 0;
            int winningCombo = -1;
            for (int combo = 0; combo < numCombos; ++combo)
            {
                int totalScore = 1;

                // if any qfi is repeated, score remains zero
                for (int q = 0; q < numQs; ++q)
                {
                    auto cs0 = results[combo * numQs + q];
                    for (int q2 = q + 1; q2 < numQs; ++q2)
                    {
                        auto cs1 = results[combo * numQs + q2];
                        if (cs0 == cs1)
                            { goto nextComboPlease; }
                    }
                }

                // no repeats; now score based on required and desired queue counts
                for (int q = 0; q < numQs; ++q)
                {
                    auto cs0 = results[combo * numQs + q];
                    auto count = suitabilityQfs[cs0].mainStruct.queueFamilyProperties.queueCount;
                    auto numQueuesDesired = qvProfile_c.get_queueVillage()[q].get_maxQueueCount();
                    auto score = std::min(count, static_cast<uint32_t>(numQueuesDesired));
                    totalScore *= score;
                }

                if (totalScore > bestScore)
                {
                    bestScore = totalScore;
                    winningCombo = combo;
                }

                nextComboPlease:
                continue;
            }

            if (winningCombo != -1)
            {
                selectedQueueFamilyProfileIdx = qvProfileIdx_c;
                for (int q = 0; q < numQs; ++q)
                {
                    auto qfi = results[winningCombo * numQs + q];
                    auto count = suitabilityQfs[qfi].mainStruct.queueFamilyProperties.queueCount;
                    auto const & qf_c = qvProfile_c.get_queueVillage()[q];
                    auto numQueuesDesired = qf_c.get_maxQueueCount();
                    auto numQueues = std::min(count, static_cast<uint32_t>(numQueuesDesired));
                    auto flags = qf_c.get_flags();

                    std::vector<float> ctdPriorities(numQueues);
                    auto const & priorities_c = qf_c.get_priorities();
                    for (int i = 0; i < numQueues; ++i)
                    {
                        ctdPriorities[i] = priorities_c[i % (priorities_c.size())];
                    }
                    qfiAllocs.queueFamilies.emplace_back(
                        QueueFamilyAlloc {qfi, numQueues, flags, std::move(ctdPriorities), qf_c.get_globalPriority()});
                }
                // we found a winner, recorded the qfi data, now bail
                log(fmt::format(". Best suitable queue family group found: {} (group #{})",
                    qvProfiles_c[selectedQueueFamilyProfileIdx].get_name(),
                    selectedQueueFamilyProfileIdx));
                break;
            }
        }

        if (selectedQueueFamilyProfileIdx == -1)
        {
            log(fmt::format(". Could not find a suitable queue family combination for profile group '{}'.", group_c.get_name()));
            // Not necessarily an error; a given profile group may just be a nice-to-have (require 0 devices).
            return {-1, {}};
        }

        return {selectedQueueFamilyProfileIdx, qfiAllocs};
    }


    void Engine::assignDevices(int groupIdx, int numDevices)
    {
        auto const & profileGroups_c = get_config().get_vkDeviceProfileGroups();
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

    void Engine::createAllVkDevices()
    {
        for (int devIdx = 0; devIdx < devices.size(); ++devIdx)
        {
            auto & device = devices[devIdx];
            device.createVkDevice();
        }
    }

    void PhysVkDevice::createVkDevice()
    {
        if (groupIdx == -1)
        {
            log(fmt::format("Device {} is not suitable for any profile group.", physicalDeviceIdx));
            return;
        }

        auto const & group_c = e->get_config().get_vkDeviceProfileGroups()[groupIdx];
        auto const & groupDesires_c = group_c.get_desires();
        auto const & profile_c = group_c.get_profiles()[profileIdx];
        auto const & profileDesires_c = profile_c.get_desires();

        std::vector<char const *> requiredDeviceExtensions;
        std::vector<char const *> requiredLayers;

        auto const & assignment = e->deviceAssignments[groupIdx];
        auto const & suitability = assignment.deviceSuitabilities[physicalDeviceIdx];
        auto const & profileCriteria = suitability.profileCritera[suitability.bestProfileIdx];
        utilizedDeviceFeatures = profileCriteria.features.copyAndReset();

        auto requireExtsAndLayersEtc = [&](auto const & criteria_c)
        {
            if (criteria_c.has_value() == false)
                { return; }

            // remaking the layers from instance creation
            for (auto const & layerName_c : criteria_c->get_layers())
            {
                auto it = std::find_if(begin(e->get_utilizedLayers()), end(e->get_utilizedLayers()),
                    [& layerName_c](auto && ae){ return layerName_c == ae; } );
                if (it != end(e->get_utilizedLayers()))
                    { requiredLayers.push_back(it->data()); }
            }
            for (auto const & deviceExtension_c : criteria_c->get_deviceExtensions())
            {
                auto it = std::find_if(begin(availableDeviceExtensions), end(availableDeviceExtensions),
                    [& deviceExtension_c](auto && ae){ return deviceExtension_c == ae.extensionName; } );
                if (it != end(availableDeviceExtensions))
                {
                    utilizedDeviceExtensions.push_back(it->extensionName);
                    requiredDeviceExtensions.push_back(it->extensionName);
                }
            }
            for (auto const & [provider_c, features_c] : criteria_c->get_features())
            {
                for (auto const & feature_c : features_c)
                {
                    if (profileCriteria.features.check(provider_c, feature_c))
                        { utilizedDeviceFeatures.set(provider_c, feature_c, VK_TRUE); }
                }
            }
        };

        requireExtsAndLayersEtc(groupDesires_c);
        requireExtsAndLayersEtc(profileDesires_c);
        requireExtsAndLayersEtc(profile_c.get_requires());

        utilizedQueueFamilies = e->deviceAssignments[groupIdx].deviceSuitabilities[physicalDeviceIdx].queueFamilyComposition;

        for (auto & re : requiredDeviceExtensions)
            { log(fmt::format("  using device extension: {}", re)); }

        /*
        log(". using features: ");
        auto & features = utilizedDeviceFeatures;
        reportFeatures(& features.features);
        void * next = features.pNext;
        while (next != nullptr)
        {
            reportFeatures(next);
            next = static_cast<VkBaseOutStructure *>(next)->pNext;
        }
        */

        log(". using queue families: ");
        for (auto const & queueFam : utilizedQueueFamilies.queueFamilies)
        {
            log(fmt::format(". . queue family index: {}\n     . . count: {}\n     . . flags: {}",
                queueFam.qfi, queueFam.count, static_cast<uint32_t>(queueFam.flags)));
        }

        auto dqcis = std::vector<VkDeviceQueueCreateInfo>(utilizedQueueFamilies.queueFamilies.size());
        for (int i = 0; i < utilizedQueueFamilies.queueFamilies.size(); ++i)
        {
            auto const & [qfi, count, flags, priorities, globalPriority] = utilizedQueueFamilies.queueFamilies[i];

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
            .pNext = & utilizedDeviceFeatures,
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

    void Engine::destroyAllVkDevices()
    {
        for (int i = 0; i < devices.size(); ++i)
        {
            devices[i].destroyVkDevice();
        }
    }

    void PhysVkDevice::destroyVkDevice()
    {
        if (device != nullptr)
        {
            vkDestroyDevice(device, nullptr);
        }
    }

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
            auto aeit = std::find_if(begin(utilizedDeviceExtensions), end(utilizedDeviceExtensions),
                [& deviceExtension](auto && elem) { return elem == deviceExtension; });
            return aeit != end(utilizedDeviceExtensions);
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
            for (auto const & qfProps : availableQueueFamilies)
            {
                queueTypesAvailable = static_cast<VkQueueFlagBits>(
                    static_cast<uenum_t>(queueTypesAvailable) |
                    static_cast<uenum_t>(qfProps.queueFamilyProperties.queueFlags));
            }
        }
        else
        {
            for (auto const & qfProps : utilizedQueueFamilies.queueFamilies)
            {
                queueTypesAvailable = static_cast<VkQueueFlagBits>(
                    static_cast<uenum_t>(queueTypesAvailable) |
                    static_cast<uenum_t>(availableQueueFamilies[qfProps.qfi].queueFamilyProperties.queueFlags));
            }
        }

        return (queueTypesAvailable & queueTypesIncluded) == queueTypesIncluded &&
               (queueTypesAvailable & queueTypesExcluded) == 0;
    }
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