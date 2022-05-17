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


        std::vector<ExtensionInfo> requiredExtensions;
        checkExtensionRequirements(requiredExtensions);
        std::vector<char const *> extsToLoad { };
        for (auto & re : requiredExtensions)
            { extsToLoad.push_back(re.name.data()); }

        VkInstanceCreateInfo vici {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = & vai,
            .enabledLayerCount = 0,         // ABOUT TO FIX THIS
            .ppEnabledLayerNames = nullptr, // ABOUT TO FIX THIS
            .enabledExtensionCount = static_cast<uint32_t>(extsToLoad.size()),
            .ppEnabledExtensionNames = extsToLoad.data()
        };

        VKR(vkCreateInstance(& vici, nullptr, & vkInstance));
        log("Vulkan instance created.");
    }

    void Engine::destroyVkInstance()
    {
        vkDestroyInstance(vkInstance, nullptr);
        vkInstance = nullptr;
    }

    void Engine::checkExtensionRequirements(std::vector<ExtensionInfo> & extensionReqs)
    {
        uint32_t glfwExtensionCount = 0;
        char const ** glfwExtensions;
        glfwExtensions = getVkExtensionsForGlfw(& glfwExtensionCount);
        for (int i = 0; i < glfwExtensionCount; ++i)
            { extensionReqs.push_back( { glfwExtensions[i] } ); }

        getVkRequiredExtensionFromConfig(extensionReqs);

        std::sort(begin(extensionReqs), end(extensionReqs),
                  [](ExtensionInfo & a, ExtensionInfo & b)
                    { return a.name < b.name; });

        if (confirmExtensions(extensionReqs) == false)
        {
            log("Some extension requirements could not be met:");
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

    void Engine::getVkRequiredExtensionFromConfig(std::vector<ExtensionInfo> & extensionReqs)
    {
        for (auto const & need : config.get_vulkanRequirements().get_extensionNeeds())
        {
            auto const & name = need.get_name();
            auto const & versionReqs = need.get_versionReqs();

            ExtensionInfo * pei;
            auto extensionIt = std::find_if(begin(extensionReqs), end(extensionReqs),
                                            [name](auto & ext){ return ext.name == name; });
            if (extensionIt != end(extensionReqs))
            {
                pei = &(* extensionIt);
            }
            else
            {
                ExtensionInfo ei = { .name = name };

                extensionReqs.push_back(std::move(ei));
                pei = & extensionReqs[extensionReqs.size() - 1];
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

    bool Engine::confirmExtensions(std::vector<ExtensionInfo> & extensionReqs)
    {
        uint32_t count = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, & count, nullptr);
        std::vector<VkExtensionProperties> extensions(count);
        vkEnumerateInstanceExtensionProperties(nullptr, & count, extensions.data());

        std::sort(begin(extensions), end(extensions),
                  [](VkExtensionProperties & a, VkExtensionProperties & b)
                    { return std::string_view(a.extensionName, strlen(a.extensionName)) <
                             std::string_view(b.extensionName, strlen(b.extensionName)); });

        int installedExtIdx = 0;
        int requiredExtIdx = 0;
        for (int installedExtIdx = 0; installedExtIdx < count; ++installedExtIdx)
        {
            auto const & ie = extensions[installedExtIdx];
            log(fmt::format("extension found: {} version: {}.{}.{}", ie.extensionName,
                VK_API_VERSION_MAJOR(ie.specVersion),
                VK_API_VERSION_MINOR(ie.specVersion),
                VK_API_VERSION_PATCH(ie.specVersion)));

            if (requiredExtIdx >= extensionReqs.size())
                { continue; }

            auto & re = extensionReqs[requiredExtIdx];
            if (ie.extensionName == re.name)
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
                        throw Ex("Absurd operator 'in' in vulkan extension version requirement.");
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
                requiredExtIdx += 1;
            }
            else if (std::string_view(ie.extensionName, strlen(ie.extensionName)) > re.name)
            {
                requiredExtIdx += 1;
            }
        }

        for (auto const & req : extensionReqs)
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
