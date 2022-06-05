#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../../gen/inc/og.hpp"
#include <map>

namespace og
{
    void Engine::initVkPhysicalDevices()
    {
        auto const & workUnits = appConfig.get_works();

        uint32_t vulkanVersion = 0;
        VKR(vkEnumerateInstanceVersion(& vulkanVersion));

        uint32_t physCount = 0;
        vkEnumeratePhysicalDevices(vkInstance, & physCount, nullptr);
        std::vector<VkPhysicalDevice> devices(physCount);
        vkEnumeratePhysicalDevices(vkInstance, & physCount, devices.data());

        availableDeviceExtensions.resize(physCount);
        availableDeviceFeatures.resize(physCount);
        availableDeviceProperties.resize(physCount);
        utilizedDeviceExtensions.resize(physCount);
        utilizedDeviceFeatures.resize(physCount);

        for (int physIdx = 0; physIdx < physCount; ++physIdx)
        {
            auto & phdev = devices[physIdx];

            log(fmt::format("Physical device {}:", physIdx));

            uint32_t count = 0;
            vkEnumerateDeviceExtensionProperties(phdev, nullptr, & count, nullptr);
            std::vector<VkExtensionProperties> & availableDevExtensions = availableDeviceExtensions[physIdx];
            vkEnumerateDeviceExtensionProperties(phdev, nullptr, & count, availableDevExtensions.data());

            for (auto const & elem : availableDevExtensions)
                { log(fmt::format("  available device extension: {} v{}", elem.extensionName, elem.specVersion)); }

            // get features for physical device
            auto & featureMap = availableDeviceFeatures[physIdx];

            VkPhysicalDeviceFeatures2 features { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            vkGetPhysicalDeviceFeatures2(phdev, & features);

            featureMap["vulkan_1_0"] = & features.features;
            void * next = features.pNext;
            while (next != nullptr)
            {
                VkStructureType structType = static_cast<VkBaseOutStructure *>(next)->sType;
                std::ostringstream ss;
                ss << HumonFormat(structType);
                auto typeCode = ss.str();

                log(fmt::format("  feature struct type: {}", typeCode));

                if (typeCode.rfind("physical_device_", 0) == 0)
                {
                    auto const startIdx = strlen("physical_device_");
                    if (auto endIdx = typeCode.find("_features", startIdx + 1) != std::string::npos)
                    {
                        std::string_view name { typeCode.data() + startIdx,
                                                endIdx - startIdx };
                        featureMap[std::string{name}] = next;
                    }
                    else
                        { throw Ex("HEY DUMMY BE MORE CAREFUL ABOUT THE PHYSICAL DEVICE FEATURES"); }
                }

                next = static_cast<VkBaseOutStructure *>(next)->pNext;
            }

            // get properties for physical device
            auto & propsMap = availableDeviceProperties[physIdx];

            VkPhysicalDeviceProperties2 props { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            vkGetPhysicalDeviceProperties2(phdev, & props);

            propsMap["vulkan_1_0"] = & props.properties;
            next = props.pNext;
            while (next != nullptr)
            {
                VkStructureType structType = static_cast<VkBaseOutStructure *>(next)->sType;
                std::ostringstream ss;
                ss << HumonFormat(structType);
                auto typeCode = ss.str();

                log(fmt::format("  property struct type: {}", typeCode));

                if (typeCode.rfind("physical_device_", 0) == 0)
                {
                    auto const startIdx = strlen("physical_device_");
                    if (auto endIdx = typeCode.find("_properties", startIdx + 1) != std::string::npos)
                    {
                        std::string_view name { typeCode.data() + startIdx,
                                                endIdx - startIdx };
                        propsMap[std::string{name}] = next;
                    }
                    else
                        { throw Ex("HEY DUMMY BE MORE CAREFUL ABOUT THE PHYSICAL DEVICE FEATURES"); }
                }

                next = static_cast<VkBaseOutStructure *>(next)->pNext;
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

            for (auto const & group : config.get_physicalVkDeviceProfiles())
            {
                auto const & groupName = group.get_name();
                auto const & profiles = group.get_profiles();
                for (int profileIdx = 0; profileIdx < profiles.size(); ++profileIdx)
                {
                    auto const & profile = profiles[profileIdx];

                    bool noGood = false;
                    auto const & criteria = profile.get_requires().get_criteria();
                    for (auto const & [lhs, op, rhs] : criteria)
                    {
                        if (rhs == "extensions")
                        {
                            if (checkExtension(lhs, op) == false)
                                { noGood = true; break; }
                        }
                        else if (rhs == "layers")
                        {
                            if (checkLayer(lhs, op) == false)
                                { noGood = true; break; }
                        }
                        else if (lhs == "vulkan")
                        {
                            if (checkVulkan(op, rhs) == false)
                                { noGood = true; break; }
                        }
                        else if (rhs == "deviceExtensions")
                        {
                            if (checkDeviceExtension(physIdx, lhs, op) == false)
                                { noGood = true; break; }
                        }
                        else if (lhs.rfind("queueTypes.", 0) == 0)
                        {
                            auto queueTypes = std::string_view { lhs.data() + 11, lhs.size() - 11 };
                            if (checkQueueTypes(physIdx, queueTypes, op, rhs) == false)
                                { noGood = true; break; }
                        }
                        else
                        {
                            // parse name
                            std::string_view provider = "vulkan_1_0";

                            auto dotIdx = lhs.find('.');
                            if (dotIdx < lhs.size() - 1)
                            {
                                provider = { lhs.data(), dotIdx++ };
                            }
                            else
                            {
                                dotIdx = 0;
                            }

                            std::string_view propOrFeat = { lhs.data() + dotIdx, lhs.size() - dotIdx };
                            bool doingFeat = false;

                            // prop or feat
                            if (propOrFeat.rfind("feat.", 0) == 0)
                            {
                                doingFeat = true;
                                propOrFeat = { propOrFeat.data() + 5, propOrFeat.size() - 5 };
                            }
                            else if (propOrFeat.rfind("prop.", 0) == 0)
                            {
                                propOrFeat = { propOrFeat.data() + 5, propOrFeat.size() - 5 };
                            }

                            if (doingFeat)
                            {
                                if (checkFeature(physIdx, provider, propOrFeat, op, rhs))
                                    { noGood = true; break; }
                            }
                            else
                            {
                                if (checkProperty(physIdx, provider, propOrFeat, op, rhs))
                                    { noGood = true; break; }
                            }
                        }
                    }

                    if (noGood == false)
                    {
                        // some kind of profiles['groupName'] = { physIdx, profileIdx }
                        log("IT WORKED MAYBE");
                        break;
                    }
                }
            }
        }
    }

    bool Engine::checkDeviceExtension(int deviceIdx, std::string_view deviceExtension, vkRequirements::reqOperator op)
    {
        if (op != vkRequirements::reqOperator::in)
            { throw Ex(fmt::format("Invalid operator {} for device extension criterion {}.", op, deviceExtension)); }

        // If we haven't made our device yet, check against available device extensions.
        // Otherwise, check against what we declared to make the device.
        std::vector<VkExtensionProperties> & availExts =
            vkInstance == nullptr ? availableDeviceExtensions[deviceIdx] : utilizedDeviceExtensions[deviceIdx];

        auto aeit = std::find_if(begin(availExts), end(availExts),
            [& deviceExtension](auto && elem) { return elem.extensionName == deviceExtension; });
        return aeit != availExts.end();
    }

    bool Engine::checkQueueTypes(int deviceIdx, std::string_view queueType, vkRequirements::reqOperator op, std::string_view value)
    {
        // If we haven't made our device yet, check against available device extensions.
        // Otherwise, check against what we declared to make the device.
        std::vector<VkQueueFamilyProperties> & queueFamilies =
            vkInstance == nullptr ? availableQueueFamilies[deviceIdx] : utilizedQueueFamilies[deviceIdx];

        if (queueType == "graphics")
        {

        }
        if (queueType == "compute")
        {

        }
        if (queueType == "transfer")
        {

        }
        if (queueType == "sparse")
        {

        }
        if (queueType == "protected")
        {

        }
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

    /*
    template <class T, typename V = std::enable_if_t<std::is_enum_v<T>>>
    bool checkMember(T const & member, vkRequirements::reqOperator op, std::string_view value)
    {
        auto const & valueConv = hu::val<T>(value);
        if (op == vkRequirements::reqOperator::has)
            { return (member & valueConv) == member; }
        if (op == vkRequirements::reqOperator::eq)
            { return member == valueConv; }
        if (op == vkRequirements::reqOperator::ne)
            { return member != valueConv; }
        throw Ex(fmt::format("Invalid operator {}.", op));
    }
    */

    //template <typename V = void>
    bool checkMember(VkBool32 const & member, vkRequirements::reqOperator op, std::string_view value)
    {
        if (op != vkRequirements::reqOperator::eq && op != vkRequirements::reqOperator::ne)
            { throw Ex(fmt::format("Invalid operator {}.", op)); }
        if (value == "true") { return member == (op == vkRequirements::reqOperator::eq ? VK_TRUE : VK_FALSE); }
        if (value == "false") { return member == (op == vkRequirements::reqOperator::ne ? VK_TRUE : VK_FALSE); }
        throw Ex(fmt::format("Invalid operator {}.", op));
    }

    // Oh teh noes! a macros!
#define CHECK_MEMBER(memberName) \
        if (propFeat == #memberName) { return checkMember(propFeatStruct->memberName, op, value); }

    // features

    static bool checkVkPropFeatStructure(VkPhysicalDeviceFeatures * propFeatStruct, std::string_view propFeat, vkRequirements::reqOperator op, std::string_view value)
    {
        CHECK_MEMBER(robustBufferAccess);
        CHECK_MEMBER(fullDrawIndexUint32);
        CHECK_MEMBER(imageCubeArray);
        CHECK_MEMBER(independentBlend);
        CHECK_MEMBER(geometryShader);
        CHECK_MEMBER(tessellationShader);
        CHECK_MEMBER(sampleRateShading);
        CHECK_MEMBER(dualSrcBlend);
        CHECK_MEMBER(logicOp);
        CHECK_MEMBER(multiDrawIndirect);
        CHECK_MEMBER(drawIndirectFirstInstance);
        CHECK_MEMBER(depthClamp);
        CHECK_MEMBER(depthBiasClamp);
        CHECK_MEMBER(fillModeNonSolid);
        CHECK_MEMBER(depthBounds);
        CHECK_MEMBER(wideLines);
        CHECK_MEMBER(largePoints);
        CHECK_MEMBER(alphaToOne);
        CHECK_MEMBER(multiViewport);
        CHECK_MEMBER(samplerAnisotropy);
        CHECK_MEMBER(textureCompressionETC2);
        CHECK_MEMBER(textureCompressionASTC_LDR);
        CHECK_MEMBER(textureCompressionBC);
        CHECK_MEMBER(occlusionQueryPrecise);
        CHECK_MEMBER(pipelineStatisticsQuery);
        CHECK_MEMBER(vertexPipelineStoresAndAtomics);
        CHECK_MEMBER(fragmentStoresAndAtomics);
        CHECK_MEMBER(shaderTessellationAndGeometryPointSize);
        CHECK_MEMBER(shaderImageGatherExtended);
        CHECK_MEMBER(shaderStorageImageExtendedFormats);
        CHECK_MEMBER(shaderStorageImageMultisample);
        CHECK_MEMBER(shaderStorageImageReadWithoutFormat);
        CHECK_MEMBER(shaderStorageImageWriteWithoutFormat);
        CHECK_MEMBER(shaderUniformBufferArrayDynamicIndexing);
        CHECK_MEMBER(shaderSampledImageArrayDynamicIndexing);
        CHECK_MEMBER(shaderStorageBufferArrayDynamicIndexing);
        CHECK_MEMBER(shaderStorageImageArrayDynamicIndexing);
        CHECK_MEMBER(shaderClipDistance);
        CHECK_MEMBER(shaderCullDistance);
        CHECK_MEMBER(shaderFloat64);
        CHECK_MEMBER(shaderInt64);
        CHECK_MEMBER(shaderInt16);
        CHECK_MEMBER(shaderResourceResidency);
        CHECK_MEMBER(shaderResourceMinLod);
        CHECK_MEMBER(sparseBinding);
        CHECK_MEMBER(sparseResidencyBuffer);
        CHECK_MEMBER(sparseResidencyImage2D);
        CHECK_MEMBER(sparseResidencyImage3D);
        CHECK_MEMBER(sparseResidency2Samples);
        CHECK_MEMBER(sparseResidency4Samples);
        CHECK_MEMBER(sparseResidency8Samples);
        CHECK_MEMBER(sparseResidency16Samples);
        CHECK_MEMBER(sparseResidencyAliased);
        CHECK_MEMBER(variableMultisampleRate);
        CHECK_MEMBER(inheritedQueries);
        throw Ex(fmt::format("Invalid member name {} for vulkan_1_0.feat.", propFeat));
    }

    static bool checkVkPropFeatStructure(VkPhysicalDeviceVulkan11Features * propFeatStruct, std::string_view propFeat, vkRequirements::reqOperator op, std::string_view value)
    {
        return true;
    }

    static bool checkVkPropFeatStructure(VkPhysicalDeviceVulkan12Features * propFeatStruct, std::string_view propFeat, vkRequirements::reqOperator op, std::string_view value)
    {
        return true;
    }

    static bool checkVkPropFeatStructure(VkPhysicalDeviceVulkan13Features * propFeatStruct, std::string_view propFeat, vkRequirements::reqOperator op, std::string_view value)
    {
        return true;
    }

    // properties

    static bool checkVkPropFeatStructure(VkPhysicalDeviceProperties * propFeatStruct, std::string_view propFeat, vkRequirements::reqOperator op, std::string_view value)
    {
        CHECK_MEMBER(apiVersion);
        CHECK_MEMBER(driverVersion);
        CHECK_MEMBER(vendorID);
        CHECK_MEMBER(deviceID);
        CHECK_MEMBER(deviceType);
        CHECK_MEMBER(limits.maxImageDimension1D);
        CHECK_MEMBER(limits.maxImageDimension2D);
        CHECK_MEMBER(limits.maxImageDimension3D);
        CHECK_MEMBER(limits.maxImageDimensionCube);
        CHECK_MEMBER(limits.maxImageArrayLayers);
        CHECK_MEMBER(limits.maxTexelBufferElements);
        CHECK_MEMBER(limits.maxUniformBufferRange);
        CHECK_MEMBER(limits.maxStorageBufferRange);
        CHECK_MEMBER(limits.maxPushConstantsSize);
        CHECK_MEMBER(limits.maxMemoryAllocationCount);
        CHECK_MEMBER(limits.maxSamplerAllocationCount);
        CHECK_MEMBER(limits.bufferImageGranularity);
        CHECK_MEMBER(limits.sparseAddressSpaceSize);
        CHECK_MEMBER(limits.maxBoundDescriptorSets);
        CHECK_MEMBER(limits.maxPerStageDescriptorSamplers);
        CHECK_MEMBER(limits.maxPerStageDescriptorUniformBuffers);
        CHECK_MEMBER(limits.maxPerStageDescriptorStorageBuffers);
        CHECK_MEMBER(limits.maxPerStageDescriptorSampledImages);
        CHECK_MEMBER(limits.maxPerStageDescriptorStorageImages);
        CHECK_MEMBER(limits.maxPerStageDescriptorInputAttachments);
        CHECK_MEMBER(limits.maxPerStageResources);
        CHECK_MEMBER(limits.maxDescriptorSetSamplers);
        CHECK_MEMBER(limits.maxDescriptorSetUniformBuffers);
        CHECK_MEMBER(limits.maxDescriptorSetUniformBuffersDynamic);
        CHECK_MEMBER(limits.maxDescriptorSetStorageBuffers);
        CHECK_MEMBER(limits.maxDescriptorSetStorageBuffersDynamic);
        CHECK_MEMBER(limits.maxDescriptorSetSampledImages);
        CHECK_MEMBER(limits.maxDescriptorSetStorageImages);
        CHECK_MEMBER(limits.maxDescriptorSetInputAttachments);
        CHECK_MEMBER(limits.maxVertexInputAttributes);
        CHECK_MEMBER(limits.maxVertexInputBindings);
        CHECK_MEMBER(limits.maxVertexInputAttributeOffset);
        CHECK_MEMBER(limits.maxVertexInputBindingStride);
        CHECK_MEMBER(limits.maxVertexOutputComponents);
        CHECK_MEMBER(limits.maxTessellationGenerationLevel);
        CHECK_MEMBER(limits.maxTessellationPatchSize);
        CHECK_MEMBER(limits.maxTessellationControlPerVertexInputComponents);
        CHECK_MEMBER(limits.maxTessellationControlPerVertexOutputComponents);
        CHECK_MEMBER(limits.maxTessellationControlPerPatchOutputComponents);
        CHECK_MEMBER(limits.maxTessellationControlTotalOutputComponents);
        CHECK_MEMBER(limits.maxTessellationEvaluationInputComponents);
        CHECK_MEMBER(limits.maxTessellationEvaluationOutputComponents);
        CHECK_MEMBER(limits.maxGeometryShaderInvocations);
        CHECK_MEMBER(limits.maxGeometryInputComponents);
        CHECK_MEMBER(limits.maxGeometryOutputComponents);
        CHECK_MEMBER(limits.maxGeometryOutputVertices);
        CHECK_MEMBER(limits.maxGeometryTotalOutputComponents);
        CHECK_MEMBER(limits.maxFragmentInputComponents);
        CHECK_MEMBER(limits.maxFragmentOutputAttachments);
        CHECK_MEMBER(limits.maxFragmentDualSrcAttachments);
        CHECK_MEMBER(limits.maxFragmentCombinedOutputResources);
        CHECK_MEMBER(limits.maxComputeSharedMemorySize);
    //uint32_t              maxComputeWorkGroupCount[3];
        CHECK_MEMBER(limits.maxComputeWorkGroupInvocations);
    //uint32_t              maxComputeWorkGroupSize[3];
        CHECK_MEMBER(limits.subPixelPrecisionBits);
        CHECK_MEMBER(limits.subTexelPrecisionBits);
        CHECK_MEMBER(limits.mipmapPrecisionBits);
        CHECK_MEMBER(limits.maxDrawIndexedIndexValue);
        CHECK_MEMBER(limits.maxDrawIndirectCount);
        CHECK_MEMBER(limits.maxSamplerLodBias);
        CHECK_MEMBER(limits.maxSamplerAnisotropy);
        CHECK_MEMBER(limits.maxViewports);
    //uint32_t              maxViewportDimensions[2];
    //float                 viewportBoundsRange[2];
        CHECK_MEMBER(limits.viewportSubPixelBits);
        CHECK_MEMBER(limits.minMemoryMapAlignment);
        CHECK_MEMBER(limits.minTexelBufferOffsetAlignment);
        CHECK_MEMBER(limits.minUniformBufferOffsetAlignment);
        CHECK_MEMBER(limits.minStorageBufferOffsetAlignment);
        CHECK_MEMBER(limits.minTexelOffset);
        CHECK_MEMBER(limits.maxTexelOffset);
        CHECK_MEMBER(limits.minTexelGatherOffset);
        CHECK_MEMBER(limits.maxTexelGatherOffset);
        CHECK_MEMBER(limits.minInterpolationOffset);
        CHECK_MEMBER(limits.maxInterpolationOffset);
        CHECK_MEMBER(limits.subPixelInterpolationOffsetBits);
        CHECK_MEMBER(limits.maxFramebufferWidth);
        CHECK_MEMBER(limits.maxFramebufferHeight);
        CHECK_MEMBER(limits.maxFramebufferLayers);
        CHECK_MEMBER(limits.framebufferColorSampleCounts);
        CHECK_MEMBER(limits.framebufferDepthSampleCounts);
        CHECK_MEMBER(limits.framebufferStencilSampleCounts);
        CHECK_MEMBER(limits.framebufferNoAttachmentsSampleCounts);
        CHECK_MEMBER(limits.maxColorAttachments);
        CHECK_MEMBER(limits.sampledImageColorSampleCounts);
        CHECK_MEMBER(limits.sampledImageIntegerSampleCounts);
        CHECK_MEMBER(limits.sampledImageDepthSampleCounts);
        CHECK_MEMBER(limits.sampledImageStencilSampleCounts);
        CHECK_MEMBER(limits.storageImageSampleCounts);
        CHECK_MEMBER(limits.maxSampleMaskWords);
        CHECK_MEMBER(limits.timestampComputeAndGraphics);
        CHECK_MEMBER(limits.timestampPeriod);
        CHECK_MEMBER(limits.maxClipDistances);
        CHECK_MEMBER(limits.maxCullDistances);
        CHECK_MEMBER(limits.maxCombinedClipAndCullDistances);
        CHECK_MEMBER(limits.discreteQueuePriorities);
    //float                 pointSizeRange[2];
    //float                 lineWidthRange[2];
        CHECK_MEMBER(limits.pointSizeGranularity);
        CHECK_MEMBER(limits.lineWidthGranularity);
        CHECK_MEMBER(limits.strictLines);
        CHECK_MEMBER(limits.standardSampleLocations);
        CHECK_MEMBER(limits.optimalBufferCopyOffsetAlignment);
        CHECK_MEMBER(limits.optimalBufferCopyRowPitchAlignment);
        CHECK_MEMBER(limits.nonCoherentAtomSize);
        CHECK_MEMBER(sparseProperties.residencyStandard2DBlockShape);
        CHECK_MEMBER(sparseProperties.residencyStandard2DMultisampleBlockShape);
        CHECK_MEMBER(sparseProperties.residencyStandard3DBlockShape);
        CHECK_MEMBER(sparseProperties.residencyAlignedMipSize);
        CHECK_MEMBER(sparseProperties.residencyNonResidentStrict);
        throw Ex(fmt::format("Invalid member name {} for vulkan_1_0.prop.", propFeat));
    }

    static bool checkVkPropFeatStructure(VkPhysicalDeviceVulkan11Properties * propFeatStruct, std::string_view propFeat, vkRequirements::reqOperator op, std::string_view value)
    {
        return true;
    }

    static bool checkVkPropFeatStructure(VkPhysicalDeviceVulkan12Properties * propFeatStruct, std::string_view propFeat, vkRequirements::reqOperator op, std::string_view value)
    {
        return true;
    }

    static bool checkVkPropFeatStructure(VkPhysicalDeviceVulkan13Properties * propFeatStruct, std::string_view propFeat, vkRequirements::reqOperator op, std::string_view value)
    {
        return true;
    }

    // don't sweat it
    #undef CHECK_MEMBER

    bool Engine::checkFeature(int deviceIdx, std::string_view provider, std::string_view feature, vkRequirements::reqOperator op, std::string_view value)
    {
        // If we haven't made our device yet, check against available features.
        // Otherwise, check against what we declared to make the device.
        std::map<std::string_view, void *> & features =
            vkInstance == nullptr ? availableDeviceFeatures[deviceIdx] : utilizedDeviceFeatures[deviceIdx];

        auto it = features.find(provider);
        if (it == features.end())
            { throw Ex(fmt::format("Invalid provider '{}' for device feature criterion '{}'.", provider, feature)); }

        bool pass = false;
        if (it->first == "vulkan_1_0")
        {
            pass = checkVkPropFeatStructure(static_cast<VkPhysicalDeviceFeatures *>(it->second), feature, op, value);
        }
        else
        {
            VkStructureType structType = static_cast<VkBaseOutStructure *>(it->second)->sType;
            switch (structType)
            {
 #define CHECK_FEAT(enumName, structName) \
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##enumName##_FEATURES: pass = checkVkPropFeatStructure(static_cast<VkPhysicalDevice##structName##Features *>(it->second), feature, op, value);
            CHECK_FEAT(VULKAN_1_1, Vulkan11);
            CHECK_FEAT(VULKAN_1_2, Vulkan12);
            CHECK_FEAT(VULKAN_1_3, Vulkan13);
            //case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES: pass = checkVkPropFeatStructure(static_cast<VkPhysicalDeviceVulkan11Features *>(it->second), feature, op, value);
            //case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES: pass = checkVkPropFeatStructure(static_cast<VkPhysicalDeviceVulkan12Features *>(it->second), feature, op, value);
            //case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES: pass = checkVkPropFeatStructure(static_cast<VkPhysicalDeviceVulkan13Features *>(it->second), feature, op, value);
#undef CHECK_FEAT
            }
        }

        return pass;
    }

    bool Engine::checkProperty(int deviceIdx, std::string_view provider, std::string_view property, vkRequirements::reqOperator op, std::string_view value)
    {
        // If we haven't made our device yet, check against available features.
        // Otherwise, check against what we declared to make the device.
        std::map<std::string_view, void *> & properties = availableDeviceProperties[deviceIdx];

        auto it = properties.find(provider);
        if (it == properties.end())
            { throw Ex(fmt::format("Invalid provider '{}' for device property criterion '{}'.", provider, property)); }

        bool pass = false;
        if (it->first == "vulkan_1_0")
        {
            pass = checkVkPropFeatStructure(static_cast<VkPhysicalDeviceProperties *>(it->second), property, op, value);
        }
        else
        {
            VkStructureType structType = static_cast<VkBaseOutStructure *>(it->second)->sType;
            switch (structType)
            {
 #define CHECK_PROP(enumName, structName) \
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##enumName##_PROPERTIES: pass = checkVkPropFeatStructure(static_cast<VkPhysicalDevice##structName##Properties *>(it->second), property, op, value);
            CHECK_PROP(VULKAN_1_1, Vulkan11);
            CHECK_PROP(VULKAN_1_2, Vulkan12);
            CHECK_PROP(VULKAN_1_3, Vulkan13);
            //case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES: pass = checkVkPropFeatStructure(static_cast<VkPhysicalDeviceVulkan11Features *>(it->second), feature, op, value);
            //case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES: pass = checkVkPropFeatStructure(static_cast<VkPhysicalDeviceVulkan12Features *>(it->second), feature, op, value);
            //case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES: pass = checkVkPropFeatStructure(static_cast<VkPhysicalDeviceVulkan13Features *>(it->second), feature, op, value);
#undef CHECK_PROP
            }
        }

        return pass;
    }


    /*
    template <>
    bool checkMember<uint32_t>(uint32_t const & member, vkRequirements::reqOperator op, std::string_view value)
    {
        uint32_t valueConv = 0;
        if (std::from_chars<uint32_t>(value.data(), value.data() + value.size(), valueConv).ec != std::errc())
            { throw Ex(fmt::format("Invalid value {} - must be a uint32_t.", op)); }
        return obeysInequality(member, valueConv, op);
    }

    template <>
    bool checkMember<int32_t>(int32_t const & member, vkRequirements::reqOperator op, std::string_view value)
    {
        int32_t valueConv = 0;
        if (std::from_chars<int32_t>(value.data(), value.data() + value.size(), valueConv).ec != std::errc())
            { throw Ex(fmt::format("Invalid value {} - must be a int32_t.", op)); }
        return obeysInequality(member, valueConv, op);
    }

    template <>
    bool checkMember<VkDeviceSize>(VkDeviceSize const & member, vkRequirements::reqOperator op, std::string_view value)
    {
        VkDeviceSize valueConv = 0;
        if (std::from_chars<VkDeviceSize>(value.data(), value.data() + value.size(), valueConv).ec != std::errc())
            { throw Ex(fmt::format("Invalid value {} - must be a VkDeviceSize.", op)); }
        return obeysInequality(member, valueConv, op);
    }

    template <>
    bool checkMember<float>(float const & member, vkRequirements::reqOperator op, std::string_view value)
    {
        float valueConv = 0;
        if (std::from_chars<float>(value.data(), value.data() + value.size(), valueConv).ec != std::errc())
            { throw Ex(fmt::format("Invalid value {} - must be a float.", op)); }
        return obeysInequality(member, valueConv, op);
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