#pragma once

#include <experimental/source_location>
#include "../gen/inc/assetPack.hpp"


namespace og
{
    using src_loc = std::experimental::source_location;

    class AssetPack
    {
    public:
        AssetPack(std::string_view packPath, int packIdx);

    private:
        assetDb::assetPack packData;
        int packIdx = -1;
    };
}

    /*
        Commands are for syncing a cache to an assetPack. There is
        an existing TOC and a new TOC, and a sync schedule will be
        built to transition. Each operation will nix a schedule entry
        and update the current state, so the schedule can be interrupted
        and throttled as desired.

        Asset cache changes are reported when the cache file can be
        unlocked, typically after a full sync is completed or a preset
        timeslice has been used.

        Commands:
            create cache                    file, slot
            open cache file                 file, slot
            schedule AIO cache->scratch     slot, offset, ptr, sz, bUrgent
                can have an operation tagged to the end
            schedule AIO scratch->cache     slot, ptr, sz
                can have an operation tagged to the end
            latch cache to pack             slot

            get imported asset cache size   file
            import asset to scratch         file, offset, sz
            copy scratch to scratch         ptr, sz, ptr




feature needs for loading to GPU from cache:
        schedule urgent AIOs from disk
            if this AIO was previously scheduled nonurgent but not yet started:
                remove it from nonurgent queue
            onDone (1):
                schedule transfer on qf that will use it
                onDone (2):
                    mark relevant features as closer to ready
                    free that staging buffer area
        block scheduling until pending urgent AIOs are done
        schedule nonurgent AIOs from disk
            onDone (1):
                schedule transfer on transfer qf
                onDone (2):
                    mark relevant features as closer to ready
                    free that staging buffer area
                set memory barrier for resource



game loop:
        ft = time.now()

        if there are changes to the feature set:
            generate a feature needs schedule

        if there are feature needs scheduled:
            generate assetDb commands, other commands
            (isn't that what a schedule is...?)

        if there are assetDb commands scheduled:
            while time.now() - ft < some_threshold:
                do assetDb commands
                    // these affect transfer queues in various queue families
                    // these also start new AIO operations
                if io_getevents(timeout = 0) returns any done loads:
                    call oneDone(1)

        if there are transfers in operation (from this or previous frames):
            if vkWaitForFences(timeout = 0) returns any done transfers:
                call onDone(2)


        ...

    */
