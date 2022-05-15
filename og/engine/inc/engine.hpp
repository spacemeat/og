#pragma once

#include <cassert>
#include <chrono>
#include <fmt/format.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../../gen/inc/og.hpp"
#include "except.hpp"


namespace og
{
    struct DisplayView
    {
        GLFWwindow * window;
        std::array<int, 2> extents;
    };

    class Engine
    {
    public:
        using duration_t = std::chrono::high_resolution_clock::duration;
        using timePoint_t = std::chrono::high_resolution_clock::time_point;

        using engineTimerType = double;
        using engineTimeDuration = std::chrono::duration<engineTimerType, std::micro>;
        using engineTimePoint = std::chrono::time_point<std::chrono::high_resolution_clock, engineTimeDuration>;

        Engine(std::string_view configPath);
        ~Engine();

        void init(engine::appConfig const & appConfig);

    private:
        void initWindowEnvironment();
        void initWindow(int view, std::string const & appName);
        void updateWindow(int view, engine::windowConfig_t const & winConfig);
        void updateWindowTitle(int view, std::string const & name);
        void destroyWindow(int view);
        bool iterateWindowsLoop();
        bool iterateWindowsLoop(int view);
        bool shouldClose(int view);

    public:
        void enterLoop();

    private:
        bool iterateLoop();

        void waitForIdleVkDevice();

    private:

        engine::engineConfig config;
        std::vector<DisplayView> views;
        int numActiveWindows = 0;
    };

    extern std::optional<Engine> e;
}

