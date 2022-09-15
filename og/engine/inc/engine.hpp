#pragma once

#include <cassert>
#include <chrono>
#include <map>
#include <fmt/format.h>

//#include "vkPhysDevice.hpp"

namespace og
{
    struct RequirementInfo
    {
        std::string_view name;
        std::array<std::tuple<abilities::op, uint32_t>, 2> versionReqs;
        bool needsMet = false;
        uint32_t installedVersion = 0;
    };

    struct NeedInfo
    {
        std::string_view lhs;
        abilities::op op;
        std::string_view rhs;
        bool needsMet = false;
    };


    class Engine
    {
    public:
        static constexpr std::array<int, 3> version = { 0, 0, 1 };

        using duration_t = std::chrono::high_resolution_clock::duration;
        using timePoint_t = std::chrono::high_resolution_clock::time_point;

        using engineTimerType = double;
        using engineTimeDuration = std::chrono::duration<engineTimerType, std::micro>;
        using engineTimePoint = std::chrono::time_point<std::chrono::high_resolution_clock, engineTimeDuration>;

        Engine(std::string configPath);
        ~Engine();

        void init();
        void shutdown();
    private:

    public:
        void enterLoop();

    private:
        bool iterateLoop();


        void waitForIdleVkDevice();

    private:
    };

    //extern std::optional<Engine> e;
}

