#include "../inc/app.hpp"
#include <fmt/format.h>
#include <thread>
#include "../../logger/inc/logger.hpp"

using namespace std::chrono_literals;

namespace og
{
    void App::initWindowEnvironment()
    {
        glfwInit();
    }

    bool App::anyWindowViews()
    {
        for (int i = 0; i < config_c.get_views().size(); ++i)
        {
            if (std::holds_alternative<app::windowConfig_t>(config_c.get_views()[i]))
                { return true; }
        }

        return false;
    }

    bool App::anyVulkanWindowViews()
    {
        for (int i = 0; i < config_c.get_views().size(); ++i)
        {
            if (auto const v = std::get_if<app::windowConfig_t>(& config_c.get_views()[i]); v && v->get_provideVulkanSurface())
                { return true; }
        }

        return false;
    }

    void App::initWindow(int view, std::string_view titleText)
    {
        auto const & appConfig = get_config();
        auto const & viewObj = config_c.get_views()[view];
        auto const & winConfig = std::get<app::windowConfig_t>(viewObj);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        auto const & extents = winConfig.get_extents();

        int count;
        GLFWmonitor * monitor = nullptr;
        if (winConfig.get_mode() == app::windowMode::fullScreen)
        {
            auto const & defaultMonitor = winConfig.get_defaultMonitor();

            GLFWmonitor ** monitors = glfwGetMonitors(& count);
            size_t idx = 0;
            if (defaultMonitor.has_value())
                { idx = * defaultMonitor; }
            monitor = monitors[idx];
        }

        // TODO: hook up GLFW's monitor config change handler

        auto window = glfwCreateWindow(extents[0], extents[1], std::string { titleText }.data(), monitor, nullptr);
        if (window == nullptr)
            { throw Ex(fmt::format("Could not create window for view {}", view)); }

        int w, h;
        glfwGetWindowSize(window, & w, & h);

        views[view] = { window, {w, h} };
        numActiveWindows += 1;
    }

    void App::updateWindow(int view, app::windowConfig_t const & newWinConfig)
    {
        auto window = views[view].window;
        auto const & viewObj = config_c.get_views()[view];
        auto const & oldWinConfig = std::get<app::windowConfig_t>(viewObj);
        auto const & oldExtents = oldWinConfig.get_extents();
        auto const & newExtents = newWinConfig.get_extents();

        if (newWinConfig.get_mode() != oldWinConfig.get_mode())
        {
            // TODO: destroy surface

            int refreshRate = GLFW_DONT_CARE;
            int count;
            GLFWmonitor * monitor = nullptr;
            if (newWinConfig.get_mode() == app::windowMode::fullScreen)
            {
                auto const & newDefaultMonitor = newWinConfig.get_defaultMonitor();
                auto const & newRefreshRate = newWinConfig.get_refreshRate();

                GLFWmonitor ** monitors = glfwGetMonitors(& count);
                size_t idx = 0;
                if (newDefaultMonitor.has_value())
                    { idx = * newDefaultMonitor; }
                monitor = monitors[idx];

                if (newRefreshRate.has_value())
                {
                    refreshRate = * newRefreshRate;
                }
            }

            glfwSetWindowMonitor(window, monitor, 0, 0,
                oldExtents[0], oldExtents[1],
                GLFW_DONT_CARE);
            std::this_thread::sleep_for(.1s); // TODO: GROSS
        }
        else
        {
            if (newExtents != oldExtents)
            {
                // TODO: destroy surface

                glfwSetWindowSize(window, newExtents[0], newExtents[1]);
                std::this_thread::sleep_for(.1s); // TODO: GROSS
            }
        }

        int w, h;
        glfwGetWindowSize(window, & w, & h);
        views[view] = { window, {w, h} };
    }

    void App::updateWindowTitle(int view, std::string_view name)
    {
        auto window = views[view].window;
        glfwSetWindowTitle(window, name.data());
    }

    void App::destroyWindow(int view)
    {
        // TODO: destroy surface

        auto & window = views[view].window;
        if (window != nullptr)
        {
            glfwDestroyWindow(window);
            window = nullptr;
            numActiveWindows -= 1;
        }
    }

    bool App::iterateWindowsLoop()
    {
        for (int i = 0; i < views.size(); ++i)
        {
            if (std::holds_alternative<app::windowConfig_t>(config_c.get_views()[i]))
            {
                if (iterateWindowsLoop(i) == false)
                    { destroyWindow(i); }
            }
        }

        assert(numActiveWindows >= 0);
        if (numActiveWindows == 0)
        {
            glfwTerminate();
            return false;
        }

        glfwPollEvents();
        return true;
    }

    bool App::iterateWindowsLoop(int view)
    {
        auto window = views[view].window;
        if (glfwWindowShouldClose(window))
            { return false; }

        return true;
    }

    bool App::shouldClose(int view)
    {
        auto & window = views[view].window;
        return glfwWindowShouldClose(window);
    }

    bool App::glfwSupportsVulkan()
    {
        return glfwVulkanSupported() == GLFW_TRUE;
    }

    // TODO: glfwGetPhysicalDevicePresentationSupport should be used to ensure preso

    void App::getVkExtensionsForGlfw(std::vector<char const *> & extensions)
    {
        uint32_t count = 0;

        char const ** exts;
        exts = glfwGetRequiredInstanceExtensions(& count);

        for (int i = 0; i < count; ++i)
        {
            extensions.push_back(exts[i]);
            log(fmt::format("Extension required for GLFW: {}\n", exts[i]));
        }
    }
}
