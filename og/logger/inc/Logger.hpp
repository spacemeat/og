#pragma once

#include <experimental/source_location>
#include <chrono>

namespace og
{
    using src_loc = std::experimental::source_location;

    template <class LoggerData, class SpeakerData, class LogPartEnumType>
    class Logger
    {
    public:
        void init();

        void log(int speakerId,
                 std::string_view message,
                 const src_loc & loc
                    = src_loc::current());

        std::chrono::time_point<std::chrono::system_clock> clockStart;
    };


    template <class SpeakerData, class LogPartEnumType>
    class Speaker
    {
    public:
        using ColorTableType = std::unordered_map<std::string, std::array<uint8_t, 3>>;

        void init(ColorTableType const & colorTable);

        std::string_view get_bgColorFormatted() const { return bgColorFormatted; }

    private:
        std::string bgColorFormatted;
    };

    template <class ListenerData, class SpeakerData, class LogPartEnumType>
    class Listener
    {
    public:
        using ColorTableType = std::unordered_map<std::string, std::array<uint8_t, 3>>;

        void init(ColorTableType const & colorTable);

        void log(int speakerId,
                 SpeakerData const & speaker,
                 std::chrono::duration<double> time,
                 std::string_view message,
                 src_loc const & loc);

    private:
        bool usingStdout = false;
        bool usingStderr = false;
        std::unique_ptr<std::ofstream> logFile;

        std::vector<std::string> colorsFormatted;
    };
}

#include "Logger.inl.hpp"
