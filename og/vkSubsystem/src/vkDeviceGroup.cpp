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

    void VkDeviceGroup::create(og::DeviceSubsystem const & pack)
    {
        idx = pack.deviceGroupIdx;
        name = pack.deviceGroupName;

        if (devices.size() == 0)
            { abilityResolver = pack.abilityResolver; }

        VkDeviceSubsystem ds;
        ds.create(pack);

        devices.push_back(std::move(ds));
    }

    void VkDeviceGroup::destroy()
    {
    }
}
