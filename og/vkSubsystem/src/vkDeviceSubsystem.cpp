#include "../inc/vkSubsystem.hpp"


namespace og
{
    VkDeviceSubsystem::VkDeviceSubsystem(AbilityResolver & abilityResolver)
    : abilityResolver(abilityResolver)
    {

    }

    VkDeviceSubsystem::~VkDeviceSubsystem()
    {
        destroy();
    }

    void VkDeviceSubsystem::create(DeviceSubsystem & pack)
    {
        physicalDeviceIdx = pack.physicalDeviceIdx;
        features = std::move(pack.features);
        pack.features = {};
        properties = std::move(pack.properties);
        pack.properties = {};
        physicalDevice = pack.physicalDevice;

        for (auto & qfpack : pack.queueFamilies)
        {
            VkQueueFamilySubsystem qfs { };
            qfs.create(qfpack);

            queueFamilies.push_back(std::move(qfs));
        }
    }

    void VkDeviceSubsystem::destroy()
    {
    }
}
