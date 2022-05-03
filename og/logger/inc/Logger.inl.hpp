#pragma once

#include "Logger.hpp"      // purely for editors
#include <iostream>
#include <fmt/format.h>
#include <chrono>


namespace og
{
    template<class LoggerData, class SpeakerData, class LogPartEnumType>
    void Logger<LoggerData, SpeakerData, LogPartEnumType>::init()
    {
        clockStart = std::chrono::system_clock::now();

        auto This = static_cast<LoggerData *>(this);
        auto & colors = This->get_colors();

        for (auto & s: This->get_speakers())
            { s.init(colors); }

        for (auto & l: This->get_listeners())
            { l.init(colors); }
    }


    template<class LoggerData, class SpeakerData, class LogPartEnumType>
    void Logger<LoggerData, SpeakerData, LogPartEnumType>::log(int speakerId,
                 std::string_view message,
                 src_loc const & loc)
    {
        auto This = static_cast<LoggerData *>(this);

        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> time = now - clockStart;

        const auto & speaker = This->get_speakers()[speakerId];

        auto & listeners = This->get_listeners();
        for (auto & list : listeners)
        {
            list.log(speakerId, speaker, time, message, loc);
        }
    }


    template<class SpeakerData, class LogPartEnumType>
    void Speaker<SpeakerData, LogPartEnumType>::init(ColorTableType const & colorTable)
    {
        auto This = static_cast<SpeakerData *>(this);

        auto const & colorName = This->get_color();
        auto && color = colorTable.at(colorName);

        bgColorFormatted = fmt::format("\x1b[48;2;{};{};{}m",
            color[0], color[1], color[2]);
    }


    template <class ListenerData, class SpeakerData, class LogPartEnumType>
    void Listener<ListenerData, SpeakerData, LogPartEnumType>::init(ColorTableType const & colorTable)
    {
        auto This = static_cast<ListenerData *>(this);
        using LogPartInt = std::underlying_type<LogPartEnumType>::type;

        // TODO: set up logFile stuff
        auto & logPath = This->get_logPath();
        if (logPath == "stdout")
            { usingStdout = true; }
        else if (logPath == "stderr")
            { usingStderr = true; }
        else
        {
            auto mode = This->get_retainHistory() ?
                std::ios_base::app :
                std::ios_base::out;
            logFile = std::make_unique<std::ofstream>(logPath, mode);
        }

        colorsFormatted.resize(
            static_cast<LogPartInt>(LogPartEnumType::numParts));

        auto & colors = This->get_colors();
        if (colors.has_value())
        {
            for (auto && c: *colors)
            {
                auto & colorName = c.second;
                auto & color = colorTable.at(colorName);
                LogPartInt i = static_cast<LogPartInt>(c.first);
                colorsFormatted[i] = fmt::format("\x1b[38;2;{};{};{}m",
                    color[0], color[1], color[2]);
            }

            colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::end)] = "\x1b[0m";
        }
    }


    template <class ListenerData, class SpeakerData, class LogPartEnumType>
    void Listener<ListenerData, SpeakerData, LogPartEnumType>::log(int speakerId,
                            SpeakerData const & speaker,
                            std::chrono::duration<double> time,
                            std::string_view message,
                            src_loc const & loc)
    {
        auto This = static_cast<ListenerData *>(this);

        auto time_placeholder = std::string_view("time");

        auto const & name = speaker.get_name();
        auto const & bgColor = speaker.get_color();
        auto const & tags = speaker.get_tags();

        using LogPartInt = std::underlying_type<LogPartEnumType>::type;

        if (static_cast<LogPartInt>(This->get_interests())
          & static_cast<LogPartInt>(tags))
        {
            auto const & format = This->get_format();
            auto bg = speaker.get_bgColorFormatted();
            auto fullMessage = fmt::format(format,
                fmt::arg("beg", bg),
                fmt::arg("ic", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::logId)]),
                fmt::arg("nc", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::logName)]),
                fmt::arg("tc", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::time)]),
                fmt::arg("mc", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::message)]),
                fmt::arg("fic", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::file)]),
                fmt::arg("lc", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::line)]),
                fmt::arg("cc", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::col)]),
                fmt::arg("fnc", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::fname)]),
                fmt::arg("end", colorsFormatted[static_cast<LogPartInt>(LogPartEnumType::end)]),
                fmt::arg("logId", speakerId),
                fmt::arg("logName", name),
                fmt::arg("time", time_placeholder),
                fmt::arg("message", message),
                fmt::arg("file", loc.file_name()),
                fmt::arg("line", loc.line()),
                fmt::arg("col", loc.column()),
                fmt::arg("fname", loc.function_name()));

            if (usingStdout)
            {
                std::cout << fullMessage;
            }
            else if (usingStderr)
            {
                std::cerr << fullMessage;
            }
            else
            {
                //(* logFile) << fullMessage;
            }
        }
    }
}

