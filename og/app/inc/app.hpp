#include <cassert>
#include <chrono>
#include <string>
#include <string_view>
#include <array>
#include <vector>
#include <optional>

#include <fmt/format.h>

#include "except.hpp"
#include "utils.hpp"
#include "troveKeeper.hpp"

#include "../../gen/inc/og.hpp"
#include "../../abilities/gen/inc/abilityLibrary_t.hpp"
#include "../../abilities/inc/providerAliasResolver.hpp"
#include "../../abilities/inc/collections.hpp"
#include "../../vkSubsystem/gen/inc/deviceConfig.hpp"
#include "../../engine/inc/engine.hpp"

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
        App(std::string const & configPath);
        ~App();

        app::appConfig const & get_config() { return config_c; }

        void init();
        void run();
        void shutdown();

    public:

        void initAbilities();
        void initViews();
        void initWindowEnvironment();
        bool anyWindowViews();
        bool anyVulkanWindowViews();
        void initWindow(int view, std::string_view titleText);
        void updateWindow(int view, app::windowConfig_t const & winConfig);
        void updateWindowTitle(int view, std::string_view text);
        void destroyWindow(int view);
        bool iterateWindowsLoop();
        bool iterateWindowsLoop(int view);
        bool shouldClose(int view);
        bool glfwSupportsVulkan();
        char const ** getVkExtensionsForGlfw(uint32_t * count);

        void createVulkanSubsystem();
    private:
        std::vector<std::tuple<std::string_view, size_t>> const & createSchedule();

    private:
        app::appConfig config_c;

        ProviderAliasResolver providerAliases;
        AbilityCollection abilities;

        std::vector<DisplayView> views;
        int numActiveWindows = 0;

        InstanceSubsystem vk;
        Engine engine;
    };

    extern std::optional<App> app;
}
