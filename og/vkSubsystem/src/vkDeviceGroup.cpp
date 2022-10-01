#include "../inc/vkSubsystem.hpp"


namespace og
{
    VkDeviceGroup::VkDeviceGroup()
    {

    }

    VkDeviceGroup::~VkDeviceGroup()
    {
        destroy();
    }

    void VkDeviceGroup::create(og::DeviceSubsystem & pack)
    {
        idx = pack.deviceGroupIdx;
        name = pack.deviceGroupName;

        if (devices.size() == 0)
            { abilityResolver = std::move(pack.abilityResolver); pack.abilityResolver = {}; }

        VkDeviceSubsystem ds { abilityResolver };
        ds.create(pack);

        devices.push_back(std::move(ds));
    }

    void VkDeviceGroup::destroy()
    {
    }
}
