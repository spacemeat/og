#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"

namespace og
{
    void Engine::initVkInstance(std::string const & appName, std::array<int, 3> const & appVersion)
    {
        VkApplicationInfo vai {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = appName.data(),
            .applicationVersion = VK_MAKE_API_VERSION(
                0, appVersion[0], appVersion[1], appVersion[2]),
            .pEngineName = "overground",
            .engineVersion = VK_MAKE_API_VERSION(
                0, Engine::version[0], Engine::version[1], Engine::version[2]
            ),
            .apiVersion = VK_API_VERSION_1_3 };

        std::vector<RequirementInfo> requiredExtensions;
        checkExtensionRequirements(requiredExtensions);
        std::vector<char const *> extsToLoad { };
        for (auto & re : requiredExtensions)
            { extsToLoad.push_back(re.name.data()); }

        std::vector<RequirementInfo> requiredLayers;
        checkLayerRequirements(requiredLayers);
        std::vector<char const *> layersToLoad { };
        for (auto & re : requiredLayers)
            { layersToLoad.push_back(re.name.data()); }

        for (auto & re : requiredExtensions)
            { log(fmt::format("using extension: {}", re.name)); }

        for (auto & re : requiredLayers)
            { log(fmt::format("using layer: {}", re.name)); }

        VkInstanceCreateInfo vici {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = & vai,
            .enabledLayerCount = static_cast<uint32_t>(layersToLoad.size()),
            .ppEnabledLayerNames = layersToLoad.data(),
            .enabledExtensionCount = static_cast<uint32_t>(extsToLoad.size()),
            .ppEnabledExtensionNames = extsToLoad.data()
        };

        VKR(vkCreateInstance(& vici, nullptr, & vkInstance));
        log("vulkan instance created.");
    }

    void Engine::destroyVkInstance()
    {
        vkDestroyInstance(vkInstance, nullptr);
        vkInstance = nullptr;
    }

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
        std::vector<vkRequirements::requirementRef> const & configReqs,
        std::vector<RequirementInfo> & returnedReqs)
    {
        for (auto const & need : configReqs)
        {
            auto const & name = need.get_name();
            auto const & versionReqs = need.get_versionReqs();

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

    void Engine::initVkDevices()
    {

    }

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

    void Engine::reqportLayers()
    {

    }


    void Engine::waitForIdleVkDevice()
    {
        // TODO: wait for idle veek device.
        // vulkanDevice.waitIdle();
    }
}
