#include <fmt/format.h>
#include "../../engine/inc/except.hpp"
#include "../inc/logger.hpp"

namespace og
{
    Logger::Logger(std::string loggerConfigPath)    // TODO: make this a string_view once humon gets a fix
    {
        auto tr = hu::Trove::fromFile(loggerConfigPath, {hu::Encoding::utf8}, hu::ErrorResponse::mum);
        if (auto t = std::get_if<hu::Trove>(& tr))
        {
            configTrove = std::move(* t);
            config = og::logger::loggerConfig(configTrove.root());

            auto & colorTable = config.get_colors();

            auto numListeners = config.get_listeners().size();
            listeners.resize(numListeners);
            for (int i = 0; i < numListeners; ++i)
            {
                listeners[i].init(config.get_listeners()[i], colorTable);
            }
        }
        else
        {
            throw Ex(fmt::format("Could not load loggers config at {}.", loggerConfigPath));
        }
    }


    void newListener::init(logger::listener const & listener, newListener::colorTableType const & colorTable)
    {
        using LogPartInt = std::underlying_type<logger::logParts>::type;

        auto const & logPath = listener.get_logPath();
        if (logPath == "stdout")
            { usingStdout = true; }
        else if (logPath == "stderr")
            { usingStderr = true; }
        else
        {
            auto mode = listener.get_retainHistory() ?
                std::ios_base::app :
                std::ios_base::out;
            logFile = std::make_unique<std::ofstream>(std::string { logPath }, mode);
        }

        colorsFormatted.resize(
            static_cast<LogPartInt>(logger::logParts::numParts));

        auto const & colors = listener.get_colors();
        if (colors.has_value())
        {
            for (auto const & c: * colors)
            {
                auto const & colorName = c.second;
                auto const & color = colorTable.at(colorName);
                LogPartInt i = static_cast<LogPartInt>(c.first);
                colorsFormatted[i] = fmt::format("\x1b[38;2;{};{};{}m",
                    color[0], color[1], color[2]);
            }

            colorsFormatted[static_cast<LogPartInt>(logger::logParts::end)] = "\x1b[0m";
        }

        auto const & bgColorName = listener.get_bgColor();
        if (bgColorName.has_value())
        {
            auto const & bgColor = colorTable.at(* bgColorName);
            bgColorFormatted = fmt::format("\x1b[48;2;{};{};{}m",
                bgColor[0], bgColor[1], bgColor[2]);
        }
        else
        {
            bgColorFormatted = "\x1b[48;2;0;0;0m";
        }
    }


    void Logger::log(logger::logTags tags,
                std::string_view message,
                const src_loc & loc)
    {
        auto time_placeholder = std::string_view("time");
        //auto & speaker = logger.get_speakers()[speakerId];
        //auto const & name = speaker.get_name();
        //auto const & tags = speaker.get_tags();

        using LogPartInt = std::underlying_type_t<logger::logParts>;

        for (int i = 0; i < listeners.size(); ++i)
        {
            auto const & genListener = config.get_listeners()[i];
            auto & listener = listeners[i];
            if (static_cast<LogPartInt>(genListener.get_interests())
              & static_cast<LogPartInt>(tags))
            {
                auto const & format = genListener.get_format();
                auto fullMessage = fmt::format(format,
                    fmt::arg("beg", listener.bgColorFormatted),
                    fmt::arg("tc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logParts::time)]),
                    fmt::arg("mc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logParts::message)]),
                    fmt::arg("fic", listener.colorsFormatted[static_cast<LogPartInt>(logger::logParts::file)]),
                    fmt::arg("lc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logParts::line)]),
                    fmt::arg("cc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logParts::col)]),
                    fmt::arg("fnc", listener.colorsFormatted[static_cast<LogPartInt>(logger::logParts::fname)]),
                    fmt::arg("end", listener.colorsFormatted[static_cast<LogPartInt>(logger::logParts::end)]),
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

    void log(logger::logTags tags,
             std::string_view message,
             const src_loc & loc)
    {
        l->log(tags, message, loc);
    }

    void log(std::string_view message,
             const src_loc & loc)
    {
        l->log(logger::logTags::status, message, loc);
    }
}
