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

    /*
    void Engine::checkExtensionRequirements(std::vector<RequirementInfo> & extensionReqs)
    {
        if (anyVulkanWindowViews())
        {
            uint32_t glfwExtensionCount = 0;
            char const ** glfwExtensions;
            glfwExtensions = getVkExtensionsForGlfw(& glfwExtensionCount);
            for (int i = 0; i < glfwExtensionCount; ++i)
                { extensionReqs.push_back( { glfwExtensions[i] } ); log(fmt::format("GLFW extension requirement: {}", glfwExtensions[i])); }
        }

        getVkRequiredReqsFromConfig(config.get_vulkanRequirements().get_extensionNeeds(), extensionReqs);

        std::sort(begin(extensionReqs), end(extensionReqs),
                  [](RequirementInfo & a, RequirementInfo & b)
                    { return a.name < b.name; });

        if (confirmExtensions(extensionReqs) == false)
        {
            log("Some vulkan extension requirements could not be met:");
            for (const auto & re : extensionReqs)
            {
                if (re.needsMet == false)
                {
                    if (re.installedVersion != 0)
                    {
                        log(fmt::format("  {} - installed version: {}.{}.{}", re.name,
                            VK_API_VERSION_MAJOR(re.installedVersion),
                            VK_API_VERSION_MINOR(re.installedVersion),
                            VK_API_VERSION_PATCH(re.installedVersion)));
                    }
                    else
                    {
                        log(fmt::format("  {} - not installed", re.name));
                    }
                }
            }
            throw Ex("Could not create vulkan instance.");
        }
    }

    void Engine::checkLayerRequirements(std::vector<RequirementInfo> & layerReqs)
    {
        getVkRequiredReqsFromConfig(config.get_vulkanRequirements().get_layerNeeds(), layerReqs);

        std::sort(begin(layerReqs), end(layerReqs),
                  [](RequirementInfo & a, RequirementInfo & b)
                    { return a.name < b.name; });

        if (confirmLayers(layerReqs) == false)
        {
            log("Some vulkan layer requirements could not be met:");
            for (const auto & re : layerReqs)
            {
                if (re.needsMet == false)
                {
                    if (re.installedVersion != 0)
                    {
                        log(fmt::format("  {} - installed version: {}.{}.{}", re.name,
                            VK_API_VERSION_MAJOR(re.installedVersion),
                            VK_API_VERSION_MINOR(re.installedVersion),
                            VK_API_VERSION_PATCH(re.installedVersion)));
                    }
                    else
                    {
                        log(fmt::format("  {} - not installed", re.name));
                    }
                }
            }
            throw Ex("Could not create vulkan instance.");
        }
    }

    void Engine::getVkRequiredReqsFromConfig(
        std::vector<vkRequirements::criteria> const & configCriteria,
        std::vector<RequirementInfo> & returnedReqs)
    {
        for (auto const & criterion : configCriteria)
        {
            auto const & name = criterion.get_name();
            auto const & versionReqs = criterion.get_versionReqs();

            RequirementInfo * pei;
            auto extensionIt = std::find_if(begin(returnedReqs), end(returnedReqs),
                                            [name](auto & ext){ return ext.name == name; });
            if (extensionIt != end(returnedReqs))
            {
                pei = &(* extensionIt);
            }
            else
            {
                RequirementInfo ei = { .name = name };

                returnedReqs.push_back(std::move(ei));
                pei = & returnedReqs[returnedReqs.size() - 1];
            }

            if (versionReqs.has_value())
            {
                if (versionReqs->size() > 0)
                {
                    auto const & [op, version] = (* versionReqs)[0];
                    pei->versionReqs[0] = { op, VK_MAKE_API_VERSION(0, version[0], version[1], version[2]) };
                }
                else
                {
                    pei->versionReqs[0] = { vkRequirements::reqOperator::ne, VK_MAKE_API_VERSION(0, 0, 0, 0) };
                }

                if (versionReqs->size() > 1)
                {
                    auto const & [op, version] = (* versionReqs)[1];
                    pei->versionReqs[1] = { op, VK_MAKE_API_VERSION(0, version[0], version[1], version[2]) };
                }
                else
                {
                    pei->versionReqs[1] = { vkRequirements::reqOperator::ne, VK_MAKE_API_VERSION(0, 0, 0, 0) };
                }
            }
        }
    }

    void Engine::checkInstanceNeeds(std::vector<NeedInfo> & needs)
    {

    }
    */
    /*
    bool Engine::confirmExtensions(std::vector<RequirementInfo> & extensionReqs)
    {
        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, & count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, & count, extensions.data());

        return confirmRequirements(extensionReqs, extensions);
    }

    bool Engine::confirmLayers(std::vector<RequirementInfo> & layerReqs)
    {
        uint32_t count = 0;
        vkEnumerateInstanceLayerProperties(& count, nullptr);
        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(& count, layers.data());

        return confirmRequirements(layerReqs, layers);
    }

    char const * getInstalledObjectName(VkExtensionProperties const & obj) { return obj.extensionName; }
    char const * getInstalledObjectName(VkLayerProperties const & obj) { return obj.layerName; }

    char const * getGeneralObjectLabel([[maybe_unused]] VkExtensionProperties const & obj) { return "extension"; }
    char const * getGeneralObjectLabel([[maybe_unused]] VkLayerProperties const & obj) { return "layer"; }

    template<class InfoType, class InstalledType>
    bool Engine::confirmRequirements(std::vector<InfoType> & reqs, std::vector<InstalledType> & installed)
    {
        std::sort(begin(installed), end(installed),
                  [](InstalledType & a, InstalledType & b)
                    {
                        auto an = getInstalledObjectName(a);
                        auto bn = getInstalledObjectName(b);
                        return std::string_view(an, strlen(an)) <
                               std::string_view(bn, strlen(bn)); });

        int requiredIdx = 0;
        for (int installedIdx = 0; installedIdx < installed.size(); ++installedIdx)
        {
            auto const & ie = installed[installedIdx];
            char const * name = getInstalledObjectName(ie);
            log(fmt::format("vulkan {} found: {} version: {}.{}.{}",
                getGeneralObjectLabel(ie), name,
                VK_API_VERSION_MAJOR(ie.specVersion),
                VK_API_VERSION_MINOR(ie.specVersion),
                VK_API_VERSION_PATCH(ie.specVersion)));

            if (requiredIdx >= reqs.size())
                { continue; }

            auto & re = reqs[requiredIdx];
            if (name == re.name)
            {
                re.needsMet = true;
                for (auto const & req : re.versionReqs)
                {
                    auto const & [op, version] = req;
                    switch(op)
                    {
                    case vkRequirements::reqOperator::eq:
                        re.needsMet = re.needsMet && ie.specVersion == version;
                        break;
                    case vkRequirements::reqOperator::ge:
                        re.needsMet = re.needsMet && ie.specVersion >= version;
                        break;
                    case vkRequirements::reqOperator::gt:
                        re.needsMet = re.needsMet && ie.specVersion > version;
                        break;
                    case vkRequirements::reqOperator::in:
                        throw Ex("Absurd operator 'in' in vulkan requirement.");
                    case vkRequirements::reqOperator::le:
                        re.needsMet = re.needsMet && ie.specVersion <= version;
                        break;
                    case vkRequirements::reqOperator::lt:
                        re.needsMet = re.needsMet && ie.specVersion < version;
                        break;
                    case vkRequirements::reqOperator::ne:
                        re.needsMet = re.needsMet && ie.specVersion != version;
                        break;
                    }
                }
                requiredIdx += 1;
            }
            else if (std::string_view(name, strlen(name)) > re.name)
            {
                requiredIdx += 1;
            }
        }

        for (auto const & req : reqs)
        {
            if (req.needsMet == false)
                { return false; }
        }

        return true;
    }
    */

    void Engine::initVkPhysicalDevices()
    {

    }

    void Engine::waitForIdleVkDevice()
    {
        // TODO: wait for idle veek device.
        // vulkanDevice.waitIdle();
    }
}
