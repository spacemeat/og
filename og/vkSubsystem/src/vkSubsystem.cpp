#include "../inc/vkSubsystem.hpp"


namespace og
{
    VkSubsystem::VkSubsystem(std::string const & configPath, ProviderAliasResolver & aliases,
                      AbilityCollection & abilities, std::string_view appName_c, version_t appVersion_c)
    : aliases(aliases), abilities(abilities), appName_c(appName_c), appVersion_c(appVersion_c),
      config_c { troves->loadAndKeep(configPath) }
    {

    }

    VkSubsystem::~VkSubsystem()
    {
        destroy();
    }

    void VkSubsystem::create(
            std::vector<std::tuple<std::string_view, std::string_view, size_t>> const & schedule,
            std::vector<std::string> const & requiredExtensions,
            std::vector<std::string> const & requiredLayers)
    {
        DeviceCreator dc(config_c, aliases, abilities, appName_c, appVersion_c);

        auto && vkPacket = dc.createVulkanSubsystem(schedule, requiredExtensions, requiredLayers);

        instance = vkPacket.instance;
        vulkanVersion = vkPacket.info.vulkanVersion;
        extensions = std::move(vkPacket.info.extensions);
        layers = std::move(vkPacket.info.layers);
        debugMessengers = std::move(vkPacket.info.debugMessengers);
        enabledValidations = std::move(vkPacket.info.enabledValidations);
        disabledValidations = std::move(vkPacket.info.disabledValidations);
        debugMessengerObjects = std::move(vkPacket.info.debugMessengerObjects);
        validationFeatures = validationFeatures;

        for (auto && vkPacketDevice : vkPacket.devices)
        {
            auto it = engineIdxs.find(vkPacketDevice.engineName);
            std::size_t idx = -1;
            if (it == end(engineIdxs))
            {
                idx = engineIdxs[vkPacketDevice.engineName] = engines.size();
                engines.emplace_back();
            }
            else
                { idx = it->second; }

            engines[idx].create(vkPacketDevice);
        }

        isCreated = true;
    }

    void VkSubsystem::destroy()
    {
        engines.clear();
        engineIdxs.clear();

        isCreated = false;
    }
}
