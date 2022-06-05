#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../inc/app.hpp"

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
        auto const & profileGroups = config.get_vkInstanceProfiles();
        auto pgit = std::find_if(begin(profileGroups), end(profileGroups),
            [& groupName](auto && elem) { return elem.get_name() == groupName; } );

        if (pgit == end(profileGroups))
            { throw Ex(fmt::format("Could not find a vkInstance profile group named '{}'", groupName)); }

        auto const & profiles = pgit->get_profiles();
        int profileIdx = -1;
        for (int i = 0; i < profiles.size(); ++i)
        {
            auto const & profile = profiles[i];

            // test profile criteria against version, inst exts, and layers
            bool noGood = false;

            auto const & vulkanVersion = profile.get_requires().get_vulkanVersion();
            if (vulkanVersion.has_value())
            {
                if (checkVulkan(* vulkanVersion) == false)
                    { noGood = true; break; }
            }

            auto const & extensions = profile.get_requires().get_extensions();
            for (auto const & extension : extensions)
            {
                if (checkExtension(extension) == false)
                    { noGood = true; break; }
            }

            auto const & layers = profile.get_requires().get_layers();
            for (auto const & layer : layers)
            {
                if (checkLayer(layer) == false)
                    { noGood = true; break; }
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

        std::vector<char const *> requiredExtensions;
        std::vector<char const *> requiredLayers;

        auto fn2 = [&](auto && criterion)
        {
            auto && [lhs, op, rhs] = criterion;
            std::string_view obj = lhs;
            if (rhs == "extensions")
            {
                auto it = std::find_if(begin(availableExtensions), end(availableExtensions),
                    [& obj](auto && ae){ return obj == ae.extensionName; } );
                if (it != end(availableExtensions))
                    { requiredExtensions.push_back(it->extensionName); }
            }
            else if (rhs == "layers")
            {
                auto it = std::find_if(begin(availableLayers), end(availableLayers),
                    [& obj](auto && ae){ return obj == ae.layerName; } );
                if (it != end(availableLayers))
                    { requiredLayers.push_back(it->layerName); }
            }
        };

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

        VkApplicationInfo vai {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = appName.data(),
            .applicationVersion = VK_MAKE_API_VERSION(
                0, appVersion.major(), appVersion.minor(), appVersion.patch()),
            .pEngineName = "overground",
            .engineVersion = VK_MAKE_API_VERSION(
                0, Engine::version[0], Engine::version[1], Engine::version[2]
            ),
            .apiVersion = VK_API_VERSION_1_3    // TODO: enable downlevel targeting?
        };

        VkInstanceCreateInfo vici {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = & vai,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data()
        };

        VKR(vkCreateInstance(& vici, nullptr, & vkInstance));
        log("vulkan instance created.");
    }

    void Engine::destroyVkInstance()
    {
        vkDestroyInstance(vkInstance, nullptr);
        vkInstance = nullptr;
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
        std::vector<VkExtensionProperties> & availExts =
            vkInstance == nullptr ? availableExtensions : utilizedExtensions;

        auto aeit = std::find_if(begin(availExts), end(availExts),
            [& extension](auto && elem) { return elem.extensionName == extension; });
        return aeit != availExts.end();
    }

    bool Engine::checkLayer(std::string_view layerName)
    {
        // If we haven't made our vkInstance yet, check against available layers.
        // Otherwise, check against what we declared to make the vkInstance.
        std::vector<VkLayerProperties> & availLayers =
            vkInstance == nullptr ? availableLayers : utilizedLayers;

        auto alit = std::find_if(begin(availLayers), end(availLayers),
            [& layerName](auto && elem) { return elem.layerName == layerName; });
        return alit != availLayers.end();
    }
}
