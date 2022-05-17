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
    using version_t = std::array<int, 3>;

    struct DisplayView
    {
        GLFWwindow * window;
        std::array<int, 2> extents;
    };

    struct RequirementInfo
    {
        std::string_view name;
        std::array<std::tuple<vkRequirements::reqOperator, uint32_t>, 2> versionReqs;
        bool needsMet = false;
        uint32_t installedVersion = 0;
    };

    struct NeedInfo
    {
        std::string_view lhs;
        vkRequirements::reqOperator op;
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

        Engine(std::string_view configPath);
        ~Engine();

        void init(std::string const & appName, std::array<int, 3> const & appVersion);
        void shutdown();
    private:
        void initViews(std::string const & appName);


        void initWindowEnvironment();
        bool anyWindowViews();
        bool anyVulkanWindowViews();
        void initWindow(int view, std::string const & appName);
        void updateWindow(int view, engine::windowConfig_t const & winConfig);
        void updateWindowTitle(int view, std::string const & name);
        void destroyWindow(int view);
        bool iterateWindowsLoop();
        bool iterateWindowsLoop(int view);
        bool shouldClose(int view);
        bool glfwSupportsVulkan();
        char const ** getVkExtensionsForGlfw(uint32_t * count);

    public:
        void enterLoop();

    private:
        bool iterateLoop();

        void initVkInstance(std::string const & appName, std::array<int, 3> const & appVersion);
        void destroyVkInstance();
        void checkExtensionRequirements(std::vector<RequirementInfo> & requiredExtensions);
        void checkLayerRequirements(std::vector<RequirementInfo> & layerReqs);
        void getVkRequiredReqsFromConfig(std::vector<vkRequirements::requirementRef> const & configReqs, std::vector<RequirementInfo> & returnedReqs);
        void checkInstanceNeeds(std::vector<NeedInfo> & needs);
        void initVkDevices();
        bool confirmExtensions(std::vector<RequirementInfo> & requiredExtensions);
        bool confirmLayers(std::vector<RequirementInfo> & requiredExtensions);
        template <class InfoType, class InstalledType>
        bool confirmRequirements(std::vector<InfoType> & reqs, std::vector<InstalledType> & installed);
        void reqportLayers();
        void waitForIdleVkDevice();

    private:

        engine::engineConfig config;
        std::vector<DisplayView> views;
        int numActiveWindows = 0;

        VkInstance vkInstance;
    };

    extern std::optional<Engine> e;
}

