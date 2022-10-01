#include "../inc/vkSubsystem.hpp"


namespace og
{
    VkEngineSubsystem::VkEngineSubsystem()
    {

    }

    VkEngineSubsystem::~VkEngineSubsystem()
    {
        destroy();
    }

    void VkEngineSubsystem::create(og::DeviceSubsystem & pack)
    {
        auto it = deviceGroupIdxs.find(pack.deviceGroupName);
        std::size_t idx = -1;
        if (it == end(deviceGroupIdxs))
        {
            idx = deviceGroupIdxs[pack.deviceGroupName] = deviceGroups.size();
            deviceGroups.emplace_back();
        }
        else
            { idx = it->second; }

        deviceGroups[idx].create(pack);
    }

    void VkEngineSubsystem::destroy()
    {
    }
}
