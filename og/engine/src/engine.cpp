#include <fmt/format.h>
#include "../inc/engine.hpp"
#include "../../logger/inc/logger.hpp"
#include "../inc/app.hpp"

namespace og
{
    Engine::Engine(std::string configPath)      // TODO: make this a string_view once humon gets a fix
    {
        //config = og::vkDeviceCreator::deviceConfig { troves->loadAndKeep(configPath) };
    }

    Engine::~Engine()
    {
        shutdown();
    }

    void Engine::init()
    {
        log("engine init");

    }

    void Engine::shutdown()
    {
    }
}
