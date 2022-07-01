#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <map>

#include "../gen/inc/physicalVkDeviceProfileGroup.hpp"

namespace og
{
    struct Features_structureChain
    {
        void init(std::vector<std::string_view> const & providers)
        {
            void ** ppNext = & baseProvider.pNext;
            for (auto const & provider : providers)
            {
                if      (provider == "vulkan_1_1") { if (vulkan_1_1) { continue; } vulkan_1_1 = std::make_unique<VkPhysicalDeviceVulkan11Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES); *ppNext = & (* vulkan_1_1); ppNext = & vulkan_1_1->pNext; }
                else if (provider == "vulkan_1_2") { if (vulkan_1_2) { continue; } vulkan_1_2 = std::make_unique<VkPhysicalDeviceVulkan12Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES); *ppNext = & (* vulkan_1_2); ppNext = & vulkan_1_2->pNext; }
                else if (provider == "vulkan_1_3") { if (vulkan_1_3) { continue; } vulkan_1_3 = std::make_unique<VkPhysicalDeviceVulkan13Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES); *ppNext = & (* vulkan_1_3); ppNext = & vulkan_1_3->pNext; }
                // ...
                else { throw Ex("Woops"); }
            }
        }

        Features_structureChain copyAndReset()
        {
            Features_structureChain newCh { baseProvider };

            void ** ppNext = & newCh.baseProvider.pNext;

            VkBaseOutStructure * pThisNext = static_cast<VkBaseOutStructure *>(baseProvider.pNext);
            while (pThisNext != nullptr)
            {
                switch(pThisNext->sType)
                {
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES: { newCh.vulkan_1_1 = std::make_unique<VkPhysicalDeviceVulkan11Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES); *ppNext = & (* newCh.vulkan_1_1); ppNext = & newCh.vulkan_1_1->pNext; break; }
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES: { newCh.vulkan_1_2 = std::make_unique<VkPhysicalDeviceVulkan12Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES); *ppNext = & (* newCh.vulkan_1_2); ppNext = & newCh.vulkan_1_2->pNext; break; }
                case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES: { newCh.vulkan_1_3 = std::make_unique<VkPhysicalDeviceVulkan13Features>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES); *ppNext = & (* newCh.vulkan_1_3); ppNext = & newCh.vulkan_1_3->pNext; break; }
                }
                pThisNext = pThisNext->pNext;
            }

            return newCh;
        }

        bool checkFeature(std::string_view feature)
        {
            return checkFeature_vulkan_1_0(feature);
        }

        bool checkFeature(VkStructureType sType, std::string_view feature)
        {
            switch(sType)
            {
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2: return checkFeature_vulkan_1_0(feature);
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES: return checkFeature_vulkan_1_1(feature);
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES: return checkFeature_vulkan_1_2(feature);
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES: return checkFeature_vulkan_1_3(feature);
            }
            return false;
        }

        bool checkFeature_vulkan_1_0(std::string_view feature)
        {
            if (feature == "robustBufferAccess"sv) { return baseProvider.features.robustBufferAccess == VK_TRUE; }
            return false;
        }

        bool checkFeature_vulkan_1_1(std::string_view feature)
        {
            if (feature == "storageBuffer16BitAccess"sv) { return vulkan_1_1 && vulkan_1_1->storageBuffer16BitAccess == VK_TRUE; }
            return false;
        }

        bool checkFeature_vulkan_1_2(std::string_view feature)
        {
            return false;
        }

        bool checkFeature_vulkan_1_3(std::string_view feature)
        {
            return false;
        }

        VkPhysicalDeviceFeatures2 baseProvider = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        std::unique_ptr<VkPhysicalDeviceVulkan11Features> vulkan_1_1;
        std::unique_ptr<VkPhysicalDeviceVulkan12Features> vulkan_1_2;
        std::unique_ptr<VkPhysicalDeviceVulkan13Features> vulkan_1_3;
        // ...
    };


    class VkPhysicalDeviceFeatures_StructureChain
    {
    public:
        VkPhysicalDeviceFeatures_StructureChain()
        {
        }

        VkPhysicalDeviceFeatures_StructureChain(
            VkPhysicalDeviceFeatures_StructureChain const & rhs)
            : providerIndexMap(rhs.providerIndexMap),
              baseStructure(rhs.baseStructure),
              chainStructures(rhs.chainStructures.size())
        {
            for (int i = 0; i < rhs.chainStructures.size(); ++i)
            {
                auto & [sTypeCur, ptr] = rhs.chainStructures[i];
#define OG_STRUCT(provider_name, struct_t, sType)                                             \
                { \
                    auto lhsPtr = static_cast<struct_t *>(std::get<1>(chainStructures[i])); \
                    lhsPtr->pNext = new struct_t { * static_cast<struct_t *>(ptr) }; \
                }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT_BASE
#undef OG_STRUCT
                if (chainStrutures.size() > 0)
                {
                    baseStructure.pNext = chainStructures[0];
                }
            }
        }

        ~VkPhysicalDeviceFeatures_StructureChain()
        {
            if (chainStructures.size() == 0)
                { return; }

            auto & [sTypeCur, ptr] = * chainStructures.begin();
            while (ptr != nullptr)
            {
#define OG_STRUCT(provider_name, struct_t, sType)                                             \
                if (sTypeCur == sType) \
                { \
                    auto typedPtr = static_cast<struct_t *>(ptr); \
                    ptr = typedPtr->pNext; \
                    delete typedPtr; \
                    continue; \
                }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT_BASE
#undef OG_STRUCT
            }
        }

        void init(std::vector<std::string_view> const & providers)
        {
            for (auto const & provider : providers)
            {
#define OG_STRUCT(provider_name, struct_t, sType)                                             \
                if (provider == #provider_name) \
                    { chainStructures.emplace_back(sType, new struct_t {sType}); }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT
                throw og::Ex(fmt::format("Unknown provider '{}'.", provider));
            }
        }

        template <class ChainedStructureType>
        ChainedStructureType & get();

    private:
        std::map<VkStructureType, int> providerIndexMap;
        VkPhysicalDeviceFeatures2 baseStructure;
        std::vector<std::tuple<VkStructureType, void *>> chainStructures;
    };

#define OG_STRUCT(provider_name, struct_t, sType)                                             \
    template <> \
    struct_t & VkPhysicalDeviceFeatures_StructureChain::get<struct_t>() \
    { \
        auto it = providerIndexMap.find(sType); \
        if (it != providerIndexMap.end()) \
        { \
            auto & [_, ptr] = chainStructures[it->second]; \
            return * (static_cast<struct_t *>(ptr)); } \
        else { throw Ex(fmt::format("Provider {} was not found in the chain.", #provider_name)); } \
    }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)                                             \
    template <> \
    struct_t & VkPhysicalDeviceFeatures_StructureChain::get<struct_t>() \
        { return baseStructure.features; }
#define OG_MEMBER(memberName)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/features_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT


    class VkPhysicalDeviceProperties_StructureChain
    {
    public:
        VkPhysicalDeviceProperties_StructureChain()
        {
        }

        ~VkPhysicalDeviceProperties_StructureChain()
        {
            if (chainStructures.size() == 0)
                { return; }

            auto & [sTypeCur, ptr] = * chainStructures.begin();
            while (ptr != nullptr)
            {
#define OG_STRUCT(provider_name, struct_t, sType)                                             \
                if (sTypeCur == sType) \
                { \
                    auto typedPtr = static_cast<struct_t *>(ptr); \
                    ptr = typedPtr->pNext; \
                    delete typedPtr; \
                    continue; \
                }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/properties_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT
            }
        }

        void init(std::vector<std::string_view> const & providers)
        {
            for (auto const & provider : providers)
            {
#define OG_STRUCT(provider_name, struct_t, sType)                                             \
                if (provider == #provider_name) \
                    { chainStructures.emplace_back(sType, new struct_t {sType}); }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/properties_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT
                throw og::Ex(fmt::format("Unknown provider '{}'.", provider));
            }
        }

        template <class ChainedStructureType>
        ChainedStructureType & get();

    private:
        std::map<VkStructureType, int> providerIndexMap;
        VkPhysicalDeviceProperties2 baseStructure;
        std::vector<std::tuple<VkStructureType, void *>> chainStructures;
    };

#define OG_STRUCT(provider_name, struct_t, sType)                                             \
    template <>   \
    struct_t & VkPhysicalDeviceProperties_StructureChain::get<struct_t>() \
    { \
        auto it = providerIndexMap.find(sType); \
        if (it != providerIndexMap.end()) \
        { \
            auto & [_, ptr] = chainStructures[it->second]; \
            return * (static_cast<struct_t *>(ptr)); } \
        else { throw Ex(fmt::format("Provider {} was not found in the chain.", #provider_name)); } \
    }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)                                             \
    template <>   \
    struct_t & VkPhysicalDeviceProperties_StructureChain::get<struct_t>() \
        { return baseStructure.properties; }
#define OG_MEMBER(memberName)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/properties_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT

    class VkQueueFamilyProperties_StructureChain
    {
    public:
        VkQueueFamilyProperties_StructureChain()
        {
        }

        ~VkQueueFamilyProperties_StructureChain()
        {
            if (chainStructures.size() == 0)
                { return; }

            auto & [sTypeCur, ptr] = * chainStructures.begin();
            while (ptr != nullptr)
            {
#define OG_STRUCT(provider_name, struct_t, sType)                                             \
                if (sTypeCur == sType) \
                { \
                    auto typedPtr = static_cast<struct_t *>(ptr); \
                    ptr = typedPtr->pNext; \
                    delete typedPtr; \
                    continue; \
                }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/queueFamilies_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT
            }
        }

        void init(std::vector<std::string_view> const & providers)
        {
            for (auto const & provider : providers)
            {
#define OG_STRUCT(provider_name, struct_t, sType)                                             \
                if (provider == #provider_name) \
                    { chainStructures.emplace_back(sType, new struct_t {sType}); }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)
#define OG_MEMBER(member_name)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/queueFamilies_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT
                throw og::Ex(fmt::format("Unknown provider '{}'.", provider));
            }
        }

        template <class ChainedStructureType>
        ChainedStructureType & get();

    private:
        std::map<VkStructureType, int> providerIndexMap;
        VkQueueFamilyProperties2 baseStructure;
        std::vector<std::tuple<VkStructureType, void *>> chainStructures;
    };

#define OG_STRUCT(provider_name, struct_t, sType)                                             \
    template <>   \
    struct_t & VkQueueFamilyProperties_StructureChain::get<struct_t>() \
    { \
        auto it = providerIndexMap.find(sType); \
        if (it != providerIndexMap.end()) \
        { \
            auto & [_, ptr] = chainStructures[it->second]; \
            return * (static_cast<struct_t *>(ptr)); } \
        else { throw Ex(fmt::format("Provider {} was not found in the chain.", #provider_name)); } \
    }
#define OG_STRUCT_BASE(provider_name, struct_t, sType)                                             \
    template <>   \
    struct_t & VkQueueFamilyProperties_StructureChain::get<struct_t>() \
        { return baseStructure.queueFamilyProperties; }
#define OG_MEMBER(memberName)
#define OG_MEMBER_ELSE(struct_name)
#define OG_STRUCT_END()
#include "../inc/queueFamilies_mac.h"
#undef OG_STRUCT_END
#undef OG_MEMBER_ELSE
#undef OG_MEMBER
#undef OG_STRUCT_BASE
#undef OG_STRUCT


    struct QueueFamilyAlloc
    {
        uint32_t qfi;
        uint32_t count;
        VkDeviceQueueCreateFlags flags;
        std::vector<float> priorities;
    };

    struct QueueFamilyComposition
    {
        std::vector<QueueFamilyAlloc> queueFamilies;
    };

    struct ProfileSpecificCriteria
    {
        VkPhysicalDeviceFeatures_StructureChain features;
        VkPhysicalDeviceProperties_StructureChain properties;
    };

    struct PhysicalDeviceSuitability
    {
        uint32_t physicalDeviceIdx;
        std::vector<ProfileSpecificCriteria> profileCritera;
        VkQueueFamilyProperties_StructureChain queueFamilies;
        uint32_t bestProfileIdx;
        uint32_t bestQueueFamilyGroupIdx;
        QueueFamilyComposition queueFamilyComposition;
    };

    struct DevProfileGroupAssignment
    {
        int groupIdx = -1;
        bool hasBeenComputed = false;
        // one for each enumerated physical device
        std::vector<PhysicalDeviceSuitability> deviceSuitabilities;
        // winning physical device indices
        std::vector<int> winningDeviceIdxs;
    };

    class PhysVkDevice  // naming is hard
    {
    public:
        PhysVkDevice();
        void init(int physicalDeviceIdx, VkPhysicalDevice phdev);
        int findBestProfileIdx(int groupIdx, engine::physicalVkDeviceProfileGroup const & profileGroup, PhysicalDeviceSuitability & suitability);
        std::tuple<int, QueueFamilyComposition> findBestQueueFamilyAllocation(int groupIdx, engine::physicalVkDeviceProfileGroup const & group);

        bool checkDeviceExtension(std::string_view deviceExtension);
        bool checkQueueTypes(VkQueueFlagBits queueTypesIncluded, VkQueueFlagBits queueTypesExcluded);

        void createVkDevice();
        void destroyVkDevice();

        int physicalDeviceIdx;
        VkPhysicalDevice physicalDevice;

        std::vector<VkExtensionProperties> availableDeviceExtensions;

        std::map<std::string_view, int> featureProviderIndexMap;
        VkPhysicalDeviceFeatures2 availableDeviceFeatures;
        std::vector<std::tuple<VkStructureType, void *>> availableFeaturesIndexable;

        std::map<std::string_view, int> propertyProviderIndexMap;
        VkPhysicalDeviceProperties2 availableDeviceProperties;
        std::vector<std::tuple<VkStructureType, void *>> availablePropertiesIndexable;

        std::vector<VkQueueFamilyProperties2> availableQueueFamilies;

        bool isAssignedToDeviceProfileGroup = false;

        int groupIdx = -1;
        int profileIdx = -1;
        VkDevice device = nullptr;

        std::vector<std::string_view> utilizedDeviceExtensions;
        VkPhysicalDeviceFeatures2 utilizedDeviceFeatures;
        std::vector<std::tuple<VkStructureType, void *>> utilizedFeaturesIndexable;
        QueueFamilyComposition utilizedQueueFamilies;
    };

}
