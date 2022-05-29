#pragma once

#include <experimental/source_location>
#include <chrono>
#include "../gen/inc/loggerConfig.hpp"

namespace og
{
    using src_loc = std::experimental::source_location;

    struct newListener
    {
        bool usingStdout = false;
        bool usingStderr = false;
        std::unique_ptr<std::ofstream> logFile;
        std::vector<std::string> colorsFormatted;
        std::string bgColorFormatted;

        using colorTableType = std::unordered_map<std::string_view, std::array<uint8_t, 3>>;

        void init(logger::listener const & listener, colorTableType const & colorTable);
    };

    class Logger
    {
    public:
        Logger(std::string_view loggerConfigPath);
        void log(logger::logTags tags,
                 std::string_view message,
                 const src_loc & loc = src_loc::current());

    private:
        hu::Trove configTrove;
        logger::loggerConfig config;
        std::chrono::time_point<std::chrono::system_clock> clockStart;
        std::vector<std::string> bgColorsFormatted;
        std::vector<newListener> listeners;
    };

    extern std::optional<Logger> l;
    void log(logger::logTags tags,
             std::string_view message,
             const src_loc & loc = src_loc::current());
    void log(std::string_view message,
             const src_loc & loc = src_loc::current());
}
