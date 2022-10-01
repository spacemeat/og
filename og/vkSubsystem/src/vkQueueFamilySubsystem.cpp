#include "../inc/vkSubsystem.hpp"


namespace og
{
    VkQueueFamilySubsystem::VkQueueFamilySubsystem()
    {

    }

    VkQueueFamilySubsystem::~VkQueueFamilySubsystem()
    {
        destroy();
    }

    void VkQueueFamilySubsystem::create(og::QueueFamilySubsystem & pack)
    {
        qfi = pack.qfi;
        count = pack.count;
        priorities = std::move(pack.priorities);
        pack.priorities = {};
        flags = pack.flags;
        globalPriority = pack.globalPriority;
        createdQueues = std::move(pack.createdQueues);
        pack.createdQueues = {};
        queueFamilyProperties = std::move(pack.queueFamilyProperties);
        pack.queueFamilyProperties = {};
    }

    void VkQueueFamilySubsystem::destroy()
    {
    }
}
