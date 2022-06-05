#include "engine.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace og
{
    struct DisplayView
    {
        GLFWwindow * window;
        std::array<int, 2> extents;
    };

    class App
    {
    public:
        App(std::string_view configPath);
        ~App();

        engine::appConfig const & get_config() { return config; }

        void init();
        void run();
        void shutdown();

    public:

        void initViews();
        void initWindowEnvironment();
        bool anyWindowViews();
        bool anyVulkanWindowViews();
        void initWindow(int view, std::string_view titleText);
        void updateWindow(int view, engine::windowConfig_t const & winConfig);
        void updateWindowTitle(int view, std::string_view text);
        void destroyWindow(int view);
        bool iterateWindowsLoop();
        bool iterateWindowsLoop(int view);
        bool shouldClose(int view);
        bool glfwSupportsVulkan();
        char const ** getVkExtensionsForGlfw(uint32_t * count);

    private:
        hu::Trove configTrove;
        engine::appConfig config;
        std::vector<DisplayView> views;
        int numActiveWindows = 0;
    };

    extern std::optional<App> app;
}
