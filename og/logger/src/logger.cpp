#include <fmt/format.h>
#include "../../engine/inc/except.hpp"
#include "../inc/logger.hpp"

namespace og
{
    Logger::Logger(std::string_view loggerConfigPath)
    {
        auto tr = hu::Trove::fromFile(loggerConfigPath);
        if (auto && t = std::get_if<hu::Trove>(& tr))
        {
            logger = og::logger::loggerConfig(t->root());

            auto & colorTable = logger.get_colors();

            auto numSpeakers = logger.get_speakers().size();
            bgColorsFormatted.resize(numSpeakers);
            for (int i = 0; i < numSpeakers; ++i)
            {
                auto & speaker = logger.get_speakers()[i];
                auto const & colorName = speaker.get_color();
                auto & color = colorTable.at(colorName);

                bgColorsFormatted[i] = fmt::format("\x1b[48;2;{};{};{}m",
                    color[0], color[1], color[2]);
            }

            auto numListeners = logger.get_listeners().size();
            listeners.resize(numListeners);
            for (int i = 0; i < numListeners; ++i)
            {
                listeners[i].init(logger.get_listeners()[i], colorTable);
            }
        }
        else
        {
            throw Ex(fmt::format("Could not load loggers config at {}.", loggerConfigPath));
        }
    }


    void newListener::init(logger::listener const & listener, newListener::colorTableType const & colorTable)
    {
        using LogPartInt = std::underlying_type<logger::logPart>::type;

        auto & logPath = listener.get_logPath();
        if (logPath == "stdout")
            { usingStdout = true; }
        else if (logPath == "stderr")
            { usingStderr = true; }
        else
        {
            auto mode = listener.get_retainHistory() ?
                std::ios_base::app :
                std::ios_base::out;
            logFile = std::make_unique<std::ofstream>(logPath, mode);
        }

        colorsFormatted.resize(
            static_cast<LogPartInt>(logger::logPart::numParts));

        auto & colors = listener.get_colors();
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

            colorsFormatted[static_cast<LogPartInt>(logger::logPart::end)] = "\x1b[0m";
        }
    }


    void Logger::log(int speakerId,
                std::string_view message,
                const src_loc & loc)
    {
        auto time_placeholder = std::string_view("time");
        auto & speaker = logger.get_speakers()[speakerId];
        auto const & name = speaker.get_name();
        auto const & tags = speaker.get_tags();

        using LogPartInt = std::underlying_type<logger::logPart>::type;

        for (int i = 0; i < listeners.size(); ++i)
        {
            auto & genListener = logger.get_listeners()[i];
            auto & listener = listeners[i];
            if (static_cast<LogPartInt>(genListener.get_interests())
              & static_cast<LogPartInt>(tags))
            {
                auto const & format = genListener.get_format();
                auto bg = bgColorsFormatted[speakerId];
                auto fullMessage = fmt::format(format,
                    fmt::arg("beg", bg),
                    fmt::arg("ic", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::logId)]),
                    fmt::arg("nc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::logName)]),
                    fmt::arg("tc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::time)]),
                    fmt::arg("mc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::message)]),
                    fmt::arg("fic", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::file)]),
                    fmt::arg("lc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::line)]),
                    fmt::arg("cc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::col)]),
                    fmt::arg("fnc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::fname)]),
                    fmt::arg("end", listener.colorsFormatted[static_cast<LogPartInt>(logger::logPart::end)]),
                    fmt::arg("logId", speakerId),
                    fmt::arg("logName", name),
                    fmt::arg("time", time_placeholder),
                    fmt::arg("message", message),
                    fmt::arg("file", loc.file_name()),
                    fmt::arg("line", loc.line()),
                    fmt::arg("col", loc.column()),
                    fmt::arg("fname", loc.function_name()));

                if (listener.usingStdout)
                {
                    std::cout << fullMessage;
                }
                else if (listener.usingStderr)
                {
                    std::cerr << fullMessage;
                }
                else
                {
                    (* listener.logFile) << fullMessage;
                }
            }
        }
    }
}
