#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../../gen/inc/og.hpp"
#include <map>

#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
    static void reportVkAvailableFeatureStruct(struct_t * propFeatStruct) \
    { \
        og::log(fmt::format(". Features - {}", #struct_name));
#define OG_MEMBER(member_name) \
            if (propFeatStruct->member_name == VK_TRUE) { og::log(fmt::format(". . {}", #member_name)); }
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END() \
    }
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT


    static void reportVkAvailableFeatureStruct(void * struc)
    {
        auto structType = static_cast<VkBaseOutStructure *>(struc)->sType;

        switch(structType)
        {
#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
        case struct_type_e: reportVkAvailableFeatureStruct(static_cast<struct_t *>(struc)); break;
#define OG_MEMBER(memberName)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT
        }
    }


#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
    static void reportVkAvailableProp(struct_t * propFeatStruct) \
    { \
        og::log(fmt::format(". Properties - {}", #struct_name));
#define OG_MEMBER(member_name) \
            og::log(fmt::format(". . {}: {}", #member_name, propFeatStruct->member_name));
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END() \
    }
#include "../inc/properties_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT

    static void cloneAndResetDeviceFeatures(VkPhysicalDeviceFeatures2 const & src, VkPhysicalDeviceFeatures2 & dst)
    {
        dst.features = src.features;
        void * next = dst.pNext;
        while (next != nullptr)
        {
            VkStructureType structType = static_cast<VkBaseOutStructure *>(next)->sType;
            switch(structType)
            {
#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
            case struct_type_e: dst.sType = type_e; dst.pNext = new struct_t { struct_type_e }; break;
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT
            }
        }
    }

    static void reportAvailableProps(void * struc)
    {
        VkStructureType structType = static_cast<VkBaseOutStructure *>(struc)->sType;
        std::ostringstream ss;
        ss << HumonFormat(structType);
        auto typeCode = ss.str();

        switch(structType)
        {
#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
        case struct_type_e: reportVkAvailableProp(static_cast<struct_t *>(struc)); break;
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END() \
        default: log(fmt::format("Unknown property structure type '{}'", structType));
#include "../inc/properties_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT
        }
    }

    static VkStructureType providerToStructureType(std::string_view provider)
    {
#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
        if (privder == #struct_name) { return struct_type_e; }
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END() \
        throw Ex(fmt::format("Unknown extension '{}'.", provider));
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT
    }

    static std::string_view structureTypeToProvider(VkStructureType sType)
    {
        switch (sType)
        {
#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
        case struct_type_e: return #struct_name##sv;
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END() \
        default: throw Ex(fmt::format("No known provider for structure type '{}'.", sType));    // TODO: stringize sType
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT
        }
    }

namespace og
{
    void Engine::initVkDevices()
    {
        auto const & workUnits = appConfig.get_works();

        uint32_t vulkanVersion = 0;
        VKR(vkEnumerateInstanceVersion(& vulkanVersion));

        uint32_t physCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, & physCount, nullptr);
        std::vector<VkPhysicalDevice> phDevices(physCount);
        vkEnumeratePhysicalDevices(vkInstance, & physCount, phDevices.data());

        availableDeviceExtensions.resize(physCount);
        availableDeviceFeatures.resize(physCount);
        availableDeviceProperties.resize(physCount);
        availableQueueFamilies.resize(physCount);

        devices.resize(physCount, nullptr);

        utilizedDeviceExtensions.resize(physCount);
        utilizedDeviceFeatures.resize(physCount);
        utilizedQueueFamilies.resize(physCount);

        for (int physIdx = 0; physIdx < physCount; ++physIdx)
        {
            auto & phdev = phDevices[physIdx];

            log(fmt::format("Physical device {}:", physIdx));

            uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(phdev, nullptr, & count, nullptr);
            std::vector<VkExtensionProperties> & availableDes = availableDeviceExtensions[physIdx];
            availableDes.resize(count);
            vkEnumerateDeviceExtensionProperties(phdev, nullptr, & count, availableDes.data());

            for (auto const & elem : availableDes)
                { log(fmt::format(". available device extension: {} v{}", elem.extensionName, elem.specVersion)); }

            // get features for physical device
            auto & features = availableDeviceFeatures[physIdx];
            features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            vkGetPhysicalDeviceFeatures2(phdev, & features);
            availableFeaturesIndexable[physIdx].push_back({features.sType, & features.features});
            featureProviderIndexMap[physIdx]["vulkan_1_0"] = 0;
            reportVkAvailableFeatureStruct(& features.features);

            void * next = features.pNext;
            while (next != nullptr)
            {
                auto sType = static_cast<VkBaseOutStructure *>(next)->sType;

                availableFeaturesIndexable[physIdx].push_back({sType, next});
                auto const provider = structureTypeToProvider(sType);
                featureProviderIndexMap[physIdx][provider] = availableFeaturesIndexable[physIdx].size();

                reportVkAvailableFeatureStruct(next);

                next = static_cast<VkBaseOutStructure *>(next)->pNext;
            }

            auto & properties = availableDeviceProperties[physIdx];
            properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            vkGetPhysicalDeviceProperties2(phdev, & properties);
            availablePropertiesIndexable[physIdx].push_back({properties.sType, & properties.properties});
            propertyProviderIndexMap[physIdx]["vulkan_1_0"] = 0;
            reportVkAvailablePropertiesStruct(& properties.properties);

            void * next = properties.pNext;
            while (next != nullptr)
            {
                auto sType = static_cast<VkBaseOutStructure *>(next)->sType;

                availablePropertiesIndexable[physIdx].push_back({sType, next});
                auto const provider = structureTypeToProvider(sType);
                propertyProviderIndexMap[physIdx][provider] = availablePropertiesIndexable[physIdx].size();

                reportVkAvailablePropertyStruct(next);

                next = static_cast<VkBaseOutStructure *>(next)->pNext;
            }

            // report queue families
            uint32_t qfCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(phdev, & qfCount, nullptr);
            std::vector<VkQueueFamilyProperties> & availableQfs = availableQueueFamilies[physIdx];
            availableQfs.resize(qfCount);
            vkGetPhysicalDeviceQueueFamilyProperties(phdev, & qfCount, availableQfs.data());

            log(". queue families:");
            for (auto const & qf : availableQfs)
            {
                std::ostringstream ss;
                ss << HumonFormat(qf.queueFlags);
                auto queueCode = ss.str();
                log(fmt::format(". . {} - {} queues", queueCode, qf.queueCount));
            }

            // match features spec'd in the config to what the device supports
            //  for each device profile group:
            //      for each profileIdx, profile:
            //          for each criteria in profile.requires:
            //              if criteria.lhs is 'vulkan':
            //                  check version
            //              if criteria.lhs is '{provider}.{property}':
            //                  if property is 'feat.{featureName}':
            //                      validateFeature(provider, featureName)
            //                  if property is 'prop.{propName}':
            //                      validateProperty(provider, propName)
            //                  else:
            //                      validateProperty(provider, property)
            //              if criteria.lhs is '{layerOrExtension}':
            //                  if criteria.rhs is 'extension':
            //                      validateExtension(criteria)
            //                  if criteria.rhs is 'deviceExtension':
            //                      validateDeviceExtension(criteria)
            //              throw "malformed criterion"
            //      if all requires are passed:
            //          profiles['groupName'] = { physIdx, profileIdx }

            for (auto const & group : config.get_vkDeviceProfileGroups())
            {
                auto const & groupName = group.get_name();
                auto const & profiles = group.get_profiles();
                log(fmt::format(". Assigning to device group '{}'.", groupName));
                int selectedProfileIdx = -1;
                for (int profileIdx = 0; profileIdx < profiles.size(); ++profileIdx)
                {
                    auto const & profile = profiles[profileIdx];
                    bool noGood = false;

                    log(fmt::format(". . Checking device profile '{}'.", profile.get_name()));

                    // NOTE: these will check against SELECTED extensions, layers, etc from instance creation.
                    auto const & vulkanVersion = profile.get_requires().get_vulkanVersion();
                    if (vulkanVersion.has_value())
                    {
                        if (checkVulkan(* vulkanVersion) == false)
                            { noGood = true; log(". . Vulkan version '{}' reqirement not met."); break; }
                    }

                    auto const & extensions = profile.get_requires().get_extensions();
                    for (auto const & extension : extensions)
                    {
                        if (checkExtension(extension) == false)
                            { noGood = true; log(fmt::format(". . Extension '{}' reqirement not met.", extension)); break; }
                    }

                    auto const & layers = profile.get_requires().get_layers();
                    for (auto const & layer : layers)
                    {
                        if (checkLayer(layer) == false)
                            { noGood = true; log(fmt::format(". . Layer '{}' reqirement not met.", layer)); break; }
                    }

                    // These will check against AVAILABLE device extensions, queueTypes, and features.

                    auto const & deviceExtensions = profile.get_requires().get_extensions();
                    for (auto const & deviceExtension : deviceExtensions)
                    {
                        if (checkDeviceExtension(physIdx, deviceExtension) == false)
                            { noGood = true; log(fmt::format(". . Device extension '{}' reqirement not met.", deviceExtension)); break; }
                    }

                    auto const & queueTypesInc = profile.get_requires().get_queueTypesIncluded();
                    auto const & queueTypesExc = profile.get_requires().get_queueTypesExcluded();
                    if (checkQueueTypes(physIdx,
                        queueTypesInc.has_value() ? * queueTypesInc : static_cast<VkQueueFlagBits>(0),
                        queueTypesExc.has_value() ? * queueTypesExc : static_cast<VkQueueFlagBits>(0))
                        == false)
                        { noGood = true; log(". . Queue types reqirement not met."); break; }


                    auto const & featuresMap = profile.get_requires().get_features();
                    for (auto const & [provider, features] : featuresMap)
                    {
                        auto const & availableFeatureMap = featureProviderIndexMap[physIdx];
                        if (auto it = availableFeatureMap.find(provider);
                            it != availableFeatureMap.end())
                        {
                            auto const [sType, strucAvail] = availableFeaturesIndexable[physIdx][it->second];

                            for (auto const & feature : features)
                            {
                                if (checkFeature(sType, strucAvail, feature) == false)
                                    { noGood = true; log(fmt::format(". . Feature '{}' reqirement not met.", feature)); break; }
                            }
                        }
                        else
                            { noGood = true; log(fmt::format(". . Feature '{}' reqirement not met.", feature)); break; }
                    }

                    auto const & propertiesMap = profile.get_requires().get_properties();
                    for (auto const & [provider, properties] : propertiesMap)
                    {
                        auto const & availablePropertyMap = propertyProviderIndexMap[physIdx];
                        if (auto it = availablePropertyMap.find(provider);
                            it != availablePropertyMap.end())
                        {
                            auto const [sType, strucAvail] = availablePropertiesIndexable[physIdx][it->second];

                            for (auto const & property : properties)
                            {
                                if (checkProperty(sType, strucAvail, property) == false)
                                    { noGood = true; log(fmt::format(". . Property '{}' reqirement not met.", property)); break; }
                            }
                        }
                        else
                            { noGood = true; log(fmt::format(". . Property '{}' reqirement not met.", property)); break; }
                    }

                    if (noGood == false)
                    {
                        selectedProfileIdx = profileIdx;
                        break;
                    }
                }

                if (selectedProfileIdx == -1)
                {
                    log(fmt::format(". Could not find a physical device for profile group {}.", groupName));
                    // Not necessarily an error; a given profile group may just be a nice-to-have (require 0 devices).
                    continue;   // next profile group
                }

                auto const & profile = profiles[selectedProfileIdx];

                // Find the best matching queue family group. We can fail device
                // creation here if there is no matching group.
                uint32_t selectedQueueFamilyGroup = -1;
                auto const & queueFamilyGroups = profile.get_queueFamilyGroups();
                for (uint32_t qfgi = 0; qfgi < queueFamilyGroups.size(); ++qfgi)
                {
                    // Populate a list of qfis that match the queue spec for each queue.
                    auto const & group = queueFamilyGroups[qfgi];
                    auto selectableQueueFamilyIndices = std::vector<std::vector<uint32_t>> (group.get_queues().size());
                    for (uint32_t queueIdx = 0; queueIdx < group.get_queues().size(); ++queueIdx)
                    {
                        auto const & queue : group.get_queues(queueIdx);
                        auto & selectable = selectableQueueFamilyIndices[queueIdx];
                        for (int devQfi = 0; devQfi < availableQueueFamilies[physIdx].count(); ++devQfi)
                        {
                            auto const & devQueueFamily = availableQueueFamilies[physIdx][devQfi];
                            if ((queue.get_include() & devQueueFamily.queueFlags) == queue.get_include() &&
                                 queue.get_requires() <= devQueueFamily.queueCount)
                            {
                                selectable.push_back(devQfi);
                            }
                        }
                    }
                    // Now find a unique qfi assignment for each queue. If there is not one, go to next group.
                    // TODO NEXT: Right here
                }

                // We have a device profile that works and a queue family profile that
                // works. Gather all the things we need to make the device, do some
                // paperwork, and make that bad boy.

                auto const & groupDesires = group.get_desires();
                auto const & profileDesires = profile.get_desires();

                std::vector<char const *> requiredDeviceExtensions;
                std::vector<char const *> requiredLayers;

                utilizedDeviceFeatures[physIdx].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                cloneAndResetDeviceFeatures(availableDeviceFeatures[physIdx], utilizedDeviceFeatures[physIdx]);

                auto requireExtsAndLayersEtc = [&](og::vkRequirements::criteria const & criteria)
                {
                    // remaking the layers from instance creation
                    for (auto const & layer : criteria.get_layers())
                    {
                        auto it = std::find_if(begin(availableLayers), end(availableLayers),
                            [& layer](auto && ae){ return layer == ae.layerName; } );
                        if (it != end(availableLayers))
                            { requiredLayers.push_back(it->layerName); }
                    }
                    for (auto const & deviceExtension : criteria.get_deviceExtensions())
                    {
                        auto it = std::find_if(begin(availableDeviceExtensions[physIdx]), end(availableDeviceExtensions[physIdx]),
                            [& deviceExtension](auto && ae){ return deviceExtension == ae.extensionName; } );
                        if (it != end(availableDeviceExtensions[physIdx]))
                            { requiredDeviceExtensions.push_back(it->extensionName); }
                    }
                    for (auto const & [provider, features] : criteria.get_features())
                    {
                        auto const & availableFeatureMap = featureProviderIndexMap[physIdx];
                        if (auto it = availableFeatureMap.find(provider);
                            it != availableFeatureMap.end())
                        {
                            void * strucAvail = availableFeaturesIndexable[physIdx][it->second];
                            void * strucUsing = utilizedFeaturesIndexable[physIdx][it->second];

                            VkStructureType sType = static_cast<VkBaseOutStructure *>(strucAvail)->sType;

                            for (auto const & feature : features)
                            {
                                if (checkFeature(sType, strucAvail, feature))
                                {
                                    setFeature(sType, strucUsing, feature);
                                }
                            }
                        }
                        else
                        {
                            // no provider by that name; next provider pls
                            continue;
                        }
                    }
                };

                if (groupDesires.has_value())
                    { requireExtsAndLayersEtc(* groupDesires); }

                if (profileDesires.has_value())
                    { requireExtsAndLayersEtc(* profileDesires); }

                requireExtsAndLayersEtc(profile.get_requires());

                // select queue families and counts by profile

                for (auto & re : requiredDeviceExtensions)
                    { log(fmt::format("  using device extension: {}", re)); }

                VkDeviceCreateInfo dci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };







                devices[physIdx] = nullptr;//vkCreateDevice(...);
                // some kind of profiles['groupName'] = { physIdx, profileIdx }
                log("IT WORKED MAYBE");

                // A physical device can be assigned to at most one profile group.
                // Break out of searchning throgh device profile groups. We have
                // our winner for this physical device.
                break;
            }
        }
    }

    bool Engine::checkDeviceExtension(int deviceIdx, std::string_view deviceExtension)
    {
        // If we haven't made our device yet, check against available device extensions.
        // Otherwise, check against what we declared to make the device.
        std::vector<VkExtensionProperties> & availExts =
            devices[deviceIdx] == nullptr ? availableDeviceExtensions[deviceIdx] : utilizedDeviceExtensions[deviceIdx];

        auto aeit = std::find_if(begin(availExts), end(availExts),
            [& deviceExtension](auto && elem) { return elem.extensionName == deviceExtension; });
        return aeit != availExts.end();
    }

    bool Engine::checkQueueTypes(int deviceIdx, VkQueueFlagBits queueTypesIncluded, VkQueueFlagBits queueTypesExcluded)
    {
        // If we haven't made our device yet, check against available device extensions.
        // Otherwise, check against what we declared to make the device.
        using uenum_t = std::underlying_type_t<VkQueueFlagBits>;
        VkQueueFlagBits queueTypesAvailable = static_cast<VkQueueFlagBits>(0);
        if (devices[deviceIdx] == nullptr)
        {
            for (auto const & qfProps : availableQueueFamilies[deviceIdx])
            {
                queueTypesAvailable = static_cast<VkQueueFlagBits>(
                    static_cast<uenum_t>(queueTypesAvailable) |
                    static_cast<uenum_t>(qfProps.queueFlags));
            }
        }
        else
        {
            for (auto const & qfProps : utilizedQueueFamilies[deviceIdx])
            {
                queueTypesAvailable = static_cast<VkQueueFlagBits>(
                    static_cast<uenum_t>(queueTypesAvailable) |
                    static_cast<uenum_t>(std::get<0>(qfProps)));
            }
        }

        return (queueTypesAvailable & queueTypesIncluded) == queueTypesIncluded &&
               (~queueTypesAvailable & queueTypesExcluded) == 0;
    }


    // --------------- checkMember<T> for checking props and features

    template <class T>
    bool checkMember(T const & member, vkRequirements::reqOperator op, std::string_view value)
    {
        auto const & valueConv = hu::val<T>::extract(value);
        if constexpr(std::is_enum_v<T>)
        {
            if (op == vkRequirements::reqOperator::has)
                { return (member & valueConv) == member; }
            if (op == vkRequirements::reqOperator::eq)
                { return member == valueConv; }
            if (op == vkRequirements::reqOperator::ne)
                { return member != valueConv; }
            throw Ex(fmt::format("Invalid operator {}.", op));
        }
        else
        {
            return obeysInequality(member, valueConv, op);
        }
    }

    //template <typename V = void>
    bool checkMember(VkBool32 const & member, vkRequirements::reqOperator op, std::string_view value)
    {
        if (op != vkRequirements::reqOperator::eq && op != vkRequirements::reqOperator::ne)
            { throw Ex(fmt::format("Invalid operator {}.", op)); }
        if (value == "true") { return member == (op == vkRequirements::reqOperator::eq ? VK_TRUE : VK_FALSE); }
        if (value == "false") { return member == (op == vkRequirements::reqOperator::ne ? VK_TRUE : VK_FALSE); }
        throw Ex(fmt::format("Invalid operator {}.", op));
    }

#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
    static bool checkFeatureMember(struct_t * featureStruct, std::string_view feature) \
    {
#define OG_MEMBER(member_name) \
        if (feature == #member_name) { return featureStruct->member_name == VK_TRUE; }
#define OG_MEMBER_ELSE(struct_name) \
        throw Ex(fmt::format("Invalid feature '{}' for struct_name.", feature));
#define OG_STRUCT_END() \
    }
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT


#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
    static bool setFeatureMember(struct_t * featureStruct, std::string_view feature) \
    {
#define OG_MEMBER(member_name) \
        if (feature == #member_name) { featureStruct->member_name = VK_TRUE; return; }
#define OG_MEMBER_ELSE(struct_name) \
        throw Ex(fmt::format("Invalid feature '{}' for struct_name.", feature));
#define OG_STRUCT_END() \
    }
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT

    bool Engine::checkFeature(VkStructureType sType, void * struc, std::string_view feature)
    {
        switch (sType)
        {
#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
        case struct_type_e: return checkFeatureMember(static_cast<struct_t *>(struc), feature);
#define OG_MEMBER(memberName)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END() \
        default: throw Ex(fmt::format("Invalid feature structure type {}.", sType));
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT
        }
    }

    void Engine::setFeature(VkStructureType sType, void * struc, std::string_view feature)
    {
        switch (sType)
        {
#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
        case struct_type_e: setFeatureMember(static_cast<struct_t *>(struc), feature);
#define OG_MEMBER(memberName)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END() \
        default: throw Ex(fmt::format("Invalid feature structure type {}.", sType));
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT
        }
    }


#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
    static bool checkPropertiesMember(struct_t * propertyStruct, std::string_view property, vkRequirements::reqOperator op, std::string_view value) \
    {
#define OG_MEMBER(member_name) \
        if (property == #member_name) { return checkMember(propertyStruct->member_name, op, value); }
#define OG_MEMBER_ELSE(struct_name) \
        throw Ex(fmt::format("Invalid property '{}' for struct_name.", property));
#define OG_STRUCT_END() \
    }
#include "../inc/properties_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT


    bool Engine::checkProperty(VkStructureType sType, void * struc, std::string_view property,
                               vkRequirements::reqOperator op, std::string_view value)
    {
        switch (sType)
        {
#define OG_STRUCT(struct_name, struct_t, struct_type_e) \
        case struct_type_e: return checkPropertyMember(static_cast<struct_t *>(struc), property);
#define OG_MEMBER(memberName)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END() \
        default: throw Ex(fmt::format("Invalid property structure type {}.", sType));
#include "../inc/properties_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT
        }
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