#include "../../logger/inc/logger.hpp"
#include "../inc/engine.hpp"


namespace og
{
    void Engine::enterLoop()
    {
        while(iterateLoop());

        waitForIdleVkDevice();
    }

    bool Engine::iterateLoop()
    {
        // TODO: Currently shuts down if there are no open windows. Eventually
        // we want to run headless too. Maybe base this behavior on a config.
        if (iterateWindowsLoop() == false)
        {
            return false;
        }

        log("das loop()!");
        return true;
    }
}
