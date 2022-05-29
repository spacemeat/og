#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"

namespace og
{
    template <class T>
    bool obeysInequality(T lhs, T rhs, vkRequirements::reqOperator op)
    {
        switch(op)
        {
        case vkRequirements::reqOperator::eq: return lhs == rhs;
        case vkRequirements::reqOperator::ne: return lhs != rhs;
        case vkRequirements::reqOperator::lt: return lhs < rhs;
        case vkRequirements::reqOperator::gt: return lhs > rhs;
        case vkRequirements::reqOperator::le: return lhs <= rhs;
        case vkRequirements::reqOperator::ge: return lhs >= rhs;
        default:
            throw Ex(fmt::format("Invalid operator '{}' for layer criteria '{}'", op, lhs));
        }
    }


    void Engine::initVkInstance(std::string_view appName, version_t appVersion)
    {
        uint32_t vulkanVersion = 0;
        VKR(vkEnumerateInstanceVersion(& vulkanVersion));

        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, & count, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, & count, availableExtensions.data());

        count = 0;
        vkEnumerateInstanceLayerProperties(& count, nullptr);
        std::vector<VkLayerProperties> availableLayers(count);
        vkEnumerateInstanceLayerProperties(& count, availableLayers.data());

        //  for each profile in profileGroup[instanceProfileGroup]:
        //      if all req is met:
        //          set best profile to profile.name
        //  gather all req and all des for winning profile
        //  make instance

        uint32_t numGlfwExts = 0;
        char const ** glfwExts = nullptr;
        if (anyVulkanWindowViews())
        {
            glfwExts = getVkExtensionsForGlfw(& numGlfwExts);
            for (uint32_t i = 0; i < numGlfwExts; ++i)
            {
                char const * extName = glfwExts[i];
                auto aeit = std::find_if(begin(availableExtensions), end(availableExtensions),
                    [& extName](auto && elem) { return elem.extensionName == extName; });
                if (aeit == availableExtensions.end())
                   { throw Ex(fmt::format("Could not find necessary GLFW extension '{}'", extName)); }
            }
        }

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
            auto const & criteria = profile.get_requires().get_criteria();
            for (auto const & [lhs, op, rhs] : criteria)
            {
                if (rhs == "extensions")
                {
                    // get extension name
                    std::string_view extName = lhs;

                    // check op, i guess
                    if (op != vkRequirements::reqOperator::in)
                        { throw Ex(fmt::format("Invalid operator {} for vkInstance profile {}.", op, profile.get_name())); }

                    // check if extension is available
                    auto aeit = std::find_if(begin(availableExtensions), end(availableExtensions),
                       [& extName](auto && elem) { return elem.extensionName == extName; });
                    if (aeit == availableExtensions.end())
                        { noGood = true; break; }
                }
                else if (rhs == "layers")
                {
                    std::string_view layerName = lhs;

                    if (op != vkRequirements::reqOperator::in)
                        { throw Ex(fmt::format("Invalid operator {} for vkInstance profile {}.", op, profile.get_name())); }

                    auto alit = std::find_if(begin(availableLayers), end(availableLayers),
                       [& layerName](auto && elem) { return elem.layerName == layerName; });
                    if (alit == availableLayers.end())
                        { noGood = true; break; }
                }
                else if (lhs == "vulkan")
                {
                    if (false == obeysInequality(version_t { vulkanVersion }.bits, version_t { rhs }.bits, op))
                        { noGood = true; break; }
                }
                else
                {
                    throw Ex(fmt::format("Invalid criterion {} for vkInstance profile {}.", lhs, profile.get_name()));
                }
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

        auto && fn = [&](auto && criterion)
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

        for (uint32_t i = 0; i < numGlfwExts; ++i)
            { requiredExtensions.push_back(glfwExts[i]); }

        if (globalDesires.has_value())
        {
            auto const & criteria = globalDesires->get_criteria();
            std::for_each(begin(criteria), end(criteria), fn);
        }
        if (groupDesires.has_value())
        {
            auto const & criteria = groupDesires->get_criteria();
            std::for_each(begin(criteria), end(criteria), fn);
        }
        if (profileDesires.has_value())
        {
            auto const & criteria = profileDesires->get_criteria();
            std::for_each(begin(criteria), end(criteria), fn);
        }

        auto const & criteria = profile.get_requires().get_criteria();
        std::for_each(begin(criteria), end(criteria), fn);

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

    void Engine::initVkPhysicalDevices()
    {

    }

    void Engine::waitForIdleVkDevice()
    {
        // TODO: wait for idle veek device.
        // vulkanDevice.waitIdle();
    }
}
