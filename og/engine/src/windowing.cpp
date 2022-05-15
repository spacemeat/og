#include "../inc/engine.hpp"
#include <fmt/format.h>
#include <thread>

using namespace std::chrono_literals;

namespace og
{
    void Engine::initWindowEnvironment()
    {
        glfwInit();
    }

    void Engine::initWindow(int view, std::string const & appName)
    {
        auto const & viewObj = config.get_views()[view];
        auto const & winConfig = std::get<engine::windowConfig_t>(viewObj);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        auto const & extents = winConfig.get_extents();

        int count;
        GLFWmonitor * monitor = nullptr;
        if (winConfig.get_mode() == engine::windowMode::fullScreen)
        {
            auto const & defaultMonitor = winConfig.get_defaultMonitor();

            GLFWmonitor ** monitors = glfwGetMonitors(& count);
            size_t idx = 0;
            if (defaultMonitor.has_value())
                { idx = * defaultMonitor; }
            monitor = monitors[idx];
        }

        // TODO: hook up GLFW's monitor config change handler

        auto window = glfwCreateWindow(extents[0], extents[1], appName.data(), monitor, nullptr);
        if (window == nullptr)
            { throw Ex(fmt::format("Could not create window for view {}", view)); }

        int w, h;
        glfwGetWindowSize(window, &w, &h);

        views[view] = { window, {w, h} };
        numActiveWindows += 1;
    }


    void Engine::updateWindow(int view, engine::windowConfig_t const & newWinConfig)
    {
        auto window = views[view].window;
        auto const & viewObj = config.get_views()[view];
        auto const & oldWinConfig = std::get<engine::windowConfig_t>(viewObj);
        auto const & oldExtents = oldWinConfig.get_extents();
        auto const & newExtents = newWinConfig.get_extents();

        if (newWinConfig.get_mode() != oldWinConfig.get_mode())
        {
            // TODO: destroy surface

            int refreshRate = GLFW_DONT_CARE;
            int count;
            GLFWmonitor * monitor = nullptr;
            if (newWinConfig.get_mode() == engine::windowMode::fullScreen)
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
        glfwGetWindowSize(window, &w, &h);
        views[view] = { window, {w, h} };
    }

    void Engine::updateWindowTitle(int view, std::string const & name)
    {
        auto window = views[view].window;
        glfwSetWindowTitle(window, name.data());
    }

    void Engine::destroyWindow(int view)
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

    bool Engine::iterateWindowsLoop()
    {
        for (int i = 0; i < views.size(); ++i)
        {
            if (std::holds_alternative<engine::windowConfig_t>(config.get_views()[i]))
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

    bool Engine::iterateWindowsLoop(int view)
    {
        auto window = views[view].window;
        if (glfwWindowShouldClose(window))
            { return false; }

        return true;
    }

    bool Engine::shouldClose(int view)
    {
        auto & window = views[view].window;
        return glfwWindowShouldClose(window);
    }
}
