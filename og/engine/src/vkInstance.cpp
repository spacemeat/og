#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../inc/app.hpp"

template <class T>
auto toNum(T t) { return static_cast<std::underlying_type_t<T>>(t); }

template <class T>
auto toEnum(std::underlying_type_t<T> n) { return static_cast<T>(n); }

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData,
    void * pUserData)
{
    auto logTags = toNum(og::logger::logTags::vulkan) |
                   toNum(og::logger::logTags::validation);
    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        { logTags |= toNum(og::logger::logTags::verbose); }
    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        { logTags |= toNum(og::logger::logTags::info); }
    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        { logTags |= toNum(og::logger::logTags::warn); }
    if (messageSeverity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        { logTags |= toNum(og::logger::logTags::error); }

    if (messageType & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
        { logTags |= toNum(og::logger::logTags::general); }
    if (messageType & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        { logTags |= toNum(og::logger::logTags::validation); }
    if (messageType & VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        { logTags |= toNum(og::logger::logTags::performance); }

    if (pCallbackData->messageIdNumber != 0)
    {
        og::log(toEnum<og::logger::logTags>(logTags), fmt::format("Vk: {} #{}: {}",
            pCallbackData->pMessageIdName, pCallbackData->messageIdNumber,
            pCallbackData->pMessage));
    }
    else
    {
        og::log(toEnum<og::logger::logTags>(logTags), fmt::format("Vk: {}: {}",
            pCallbackData->pMessageIdName, pCallbackData->pMessage));
    }
    // TODO: labels in queues and command buffers, and objects

    return VK_FALSE;
}


static VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo(
    og::vkRequirements::debugUtilsMessenger_t const & cfg)
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
    void Engine::initVkInstance()
    {
        auto const & appConfig = app->get_config();
        std::string_view appName = appConfig.get_name();
        auto appVersion = version_t { appConfig.get_version() };

        uint32_t vulkanVersion = 0;
        VKR(vkEnumerateInstanceVersion(& vulkanVersion));

        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, & count, nullptr);
        availableExtensions.resize(count);
        vkEnumerateInstanceExtensionProperties(nullptr, & count, availableExtensions.data());

        for (auto const & elem : availableExtensions)
            { log(fmt::format("available instance extension: {} v{}", elem.extensionName, elem.specVersion)); }

        count = 0;
        vkEnumerateInstanceLayerProperties(& count, nullptr);
        availableLayers.resize(count);
        vkEnumerateInstanceLayerProperties(& count, availableLayers.data());

        for (auto const & elem : availableLayers)
            { log(fmt::format("available layer: {} v{}", elem.layerName, elem.implementationVersion)); }

        // get GLFW extensions
        uint32_t numGlfwExts = 0;
        char const ** glfwExts = nullptr;
        if (app->anyVulkanWindowViews())
        {
            glfwExts = app->getVkExtensionsForGlfw(& numGlfwExts);
            for (uint32_t i = 0; i < numGlfwExts; ++i)
            {
                char const * extName = glfwExts[i];
                auto aeit = std::find_if(begin(availableExtensions), end(availableExtensions),
                    [& extName](auto && elem) { return elem.extensionName == extName; });
                if (aeit == availableExtensions.end())
                   { throw Ex(fmt::format("Could not find necessary GLFW extension '{}'", extName)); }
            }
        }

        // get best vkInstance profile in requested group
        auto const & groupName = config.get_useVkInstanceProfileGroup();
        auto const & profileGroups = config.get_vkInstanceProfileGroups();
        auto pgit = std::find_if(begin(profileGroups), end(profileGroups),
            [& groupName](auto && elem) { return elem.get_name() == groupName; } );

        if (pgit == end(profileGroups))
            { throw Ex(fmt::format("Could not find a vkInstance profile group named '{}'", groupName)); }

        auto const & profileGroup = * pgit;
        auto const & profiles = profileGroup.get_profiles();
        int profileIdx = -1;
        for (int i = 0; i < profiles.size(); ++i)
        {
            auto const & profile = profiles[i];

            // test profile criteria against version, inst exts, and layers
            bool noGood = false;

            // NOTE: these will check against AVAILABLE extensions, layers, etc.
            auto const & vulkanVersion = profile.get_requires().get_vulkanVersion();
            if (vulkanVersion.has_value())
            {
                if (checkVulkan(* vulkanVersion) == false)
                    { noGood = true; log("Vulkan version {} reqirement not met."); break; }
            }

            auto const & extensions = profile.get_requires().get_extensions();
            for (auto const & extension : extensions)
            {
                if (checkExtension(extension) == false)
                    { noGood = true; log(fmt::format("Extension {} reqirement not met.", extension)); break; }
            }

            auto const & layers = profile.get_requires().get_layers();
            for (auto const & layer : layers)
            {
                if (checkLayer(layer) == false)
                    { noGood = true; log(fmt::format("Layer {} reqirement not met.", layer)); break; }
            }

            if (noGood == false)
            {
                profileIdx = i;
                break;
            }
        }

        if (profileIdx < 0)
            { throw Ex(fmt::format("Unable to meet selection criteria for any vkInstance profile in {}.", groupName)); }

        auto const & profile = profiles[profileIdx];
        auto const & globalDesires = config.get_vkInstanceDesires();
        auto const & groupDesires = pgit->get_desires();
        auto const & profileDesires = profile.get_desires();

        auto const & vulkanVersionStr = profile.get_requires().get_vulkanVersion();
        auto vkVersion = vulkanVersionStr.has_value()
                       ? version_t {* vulkanVersionStr}.bits
                       : VK_API_VERSION_1_3;

        std::vector<char const *> requiredExtensions;
        std::vector<char const *> requiredLayers;

        auto requireExtsAndLayers = [&](og::vkRequirements::criteria const & criteria)
        {
            for (auto const & extension : criteria.get_extensions())
            {
                auto it = std::find_if(begin(availableExtensions), end(availableExtensions),
                    [& extension](auto && ae){ return extension == ae.extensionName; } );
                if (it != end(availableExtensions))
                    { requiredExtensions.push_back(it->extensionName); }
            }
            for (auto const & layer : criteria.get_layers())
            {
                auto it = std::find_if(begin(availableLayers), end(availableLayers),
                    [& layer](auto && ae){ return layer == ae.layerName; } );
                if (it != end(availableLayers))
                    { requiredLayers.push_back(it->layerName); }
            }
        };

        for (uint32_t i = 0; i < numGlfwExts; ++i)
        {
            log(fmt::format("GLFW needs extension: {}", glfwExts[i]));
            requiredExtensions.push_back(glfwExts[i]);
        }

        if (globalDesires.has_value())
            { requireExtsAndLayers(* globalDesires); }

        if (groupDesires.has_value())
            { requireExtsAndLayers(* groupDesires); }

        if (profileDesires.has_value())
            { requireExtsAndLayers(* profileDesires); }

        requireExtsAndLayers(profile.get_requires());

        for (auto & re : requiredExtensions)
            { log(fmt::format("using extension: {}", re)); }

        for (auto & re : requiredLayers)
            { log(fmt::format("using layer: {}", re)); }

        void * createInfo_pNext = nullptr;
        // debug messengers specifically for vkCreateInstance/vkDestroyInstance
        std::vector<VkDebugUtilsMessengerCreateInfoEXT> dbgMsgrs(profileGroup.get_debugUtilsMessengers().size());
        for (int i = 0; i < dbgMsgrs.size(); ++i)
        {
            auto const & cfg = profileGroup.get_debugUtilsMessengers()[i];
            dbgMsgrs[i] = std::move(makeDebugMessengerCreateInfo(cfg));
            dbgMsgrs[i].pNext = nullptr;
            if (i > 0)
                { dbgMsgrs[i - 1].pNext = & dbgMsgrs[i]; }
        }
        if (dbgMsgrs.size() > 0)
            { createInfo_pNext = & dbgMsgrs[0]; }

        std::optional<VkValidationFeaturesEXT> valFeats;
        auto const & valFeatCfg = profileGroup.get_validationFeatures();
        if (valFeatCfg.has_value())
        {
            valFeats = VkValidationFeaturesEXT {
                .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
                .pNext = createInfo_pNext,
                .enabledValidationFeatureCount = static_cast<uint32_t>(valFeatCfg->get_enabled().size()),
                .pEnabledValidationFeatures = valFeatCfg->get_enabled().data(),
                .disabledValidationFeatureCount = static_cast<uint32_t>(valFeatCfg->get_disabled().size()),
                .pDisabledValidationFeatures = valFeatCfg->get_disabled().data()
            };
            createInfo_pNext = & (* valFeats);
        }

        VkApplicationInfo vai {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = appName.data(),
            .applicationVersion = VK_MAKE_API_VERSION(
                0, appVersion.major(), appVersion.minor(), appVersion.patch()),
            .pEngineName = "overground",
            .engineVersion = VK_MAKE_API_VERSION(
                0, Engine::version[0], Engine::version[1], Engine::version[2]
            ),
            .apiVersion = vkVersion
        };

        VkInstanceCreateInfo vici {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = createInfo_pNext,
            .pApplicationInfo = & vai,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()
        };

        VKR(vkCreateInstance(& vici, nullptr, & vkInstance));
        log("vulkan instance created.");

        createDebugMessengers(dbgMsgrs);
    }

    void Engine::destroyVkInstance()
    {
        vkDestroyInstance(vkInstance, nullptr);
        vkInstance = nullptr;
    }

    void Engine::createDebugMessengers(std::vector<VkDebugUtilsMessengerCreateInfoEXT> const & dbgMsgrs)
    {
        vkDebugMessengers.resize(dbgMsgrs.size());
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkCreateDebugUtilsMessengerEXT");
        for (int i = 0; i < dbgMsgrs.size(); ++i)
        {
            VKR(func(vkInstance, & dbgMsgrs[i], nullptr, & vkDebugMessengers[i]));
        }
    }

    void Engine::destroyDebugMessengers()
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vkInstance, "vkDestroyDebugUtilsMessengerEXT");
        for (int i = 0; i < vkDebugMessengers.size(); ++i)
        {
            func(vkInstance, vkDebugMessengers[vkDebugMessengers.size() - i - 1], nullptr);
        }
    }

    bool Engine::checkVulkan(std::string_view vulkanVersionReq)
    {
        uint32_t vulkanVersion = 0;
        VKR(vkEnumerateInstanceVersion(& vulkanVersion));
        return version_t { vulkanVersion }.bits >= version_t { vulkanVersionReq }.bits;
    }

    bool Engine::checkExtension(std::string_view extension)
    {
        // If we haven't made our vkInstance yet, check against available extensions.
        // Otherwise, check against what we declared to make the vkInstance.
        if (vkInstance == nullptr)
        {
            auto aeit = std::find_if(begin(availableExtensions), end(availableExtensions),
                [& extension](auto && elem) { return elem.extensionName == extension; });
            return aeit != availableExtensions.end();

        }
        else
        {
            auto aeit = std::find_if(begin(utilizedExtensions), end(utilizedExtensions),
                [& extension](auto && elem) { return elem == extension; });
            return aeit != utilizedExtensions.end();
        }
    }

    bool Engine::checkLayer(std::string_view layerName)
    {
        // If we haven't made our vkInstance yet, check against available layers.
        // Otherwise, check against what we declared to make the vkInstance.
        if (vkInstance == nullptr)
        {
            auto aeit = std::find_if(begin(availableLayers), end(availableLayers),
                [& layerName](auto && elem) { return elem.layerName == layerName; });
            return aeit != availableLayers.end();

        }
        else
        {
            auto aeit = std::find_if(begin(utilizedLayers), end(utilizedLayers),
                [& layerName](auto && elem) { return elem == layerName; });
            return aeit != utilizedLayers.end();
        }
    }
}
