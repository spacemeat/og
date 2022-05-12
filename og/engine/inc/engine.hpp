#pragma once

#include "../../gen/inc/og.hpp"
#include "../../logger/inc/logger.hpp"
#include "except.hpp"


namespace og
{

    class Engine
    {
    public:
        Engine(std::string_view configPath);

        void loop();

    private:
        engine::engineConfig config;
    };

    extern std::optional<Engine> e;
}

