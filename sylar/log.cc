#include "log.h"
#include <iostream>
#include <functional>

namespace sylar
{
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)                \
    case LogLevel::Level::name: \
        return #name;           \
        break;
            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX
        default:
            return "UNKOWN";
            break;
        }
        return "UNKOWN";
    }

    LogEvent::LogEvent(const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time) : m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time) {}

    Logger::Logger(const std::string name) : m_name(name), m_level(LogLevel::DEBUG)
    {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}\t%t\t%F\t[%p]\t[%c]\t%f:%l\t%m %n"));
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        if (!appender->getFormatter())
        {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            auto self = shared_from_this();
            for (auto &i : m_appenders)
            {
                i->log(self, level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::Level::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::Level::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::Level::WARN, event);
    }

    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::Level::ERROR, event);
    }

    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::Level::FATAL, event);
    }

    FileLogAppender::FileLogAppender(const std::string &filename) : m_filename(filename) {}

    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            m_filestream << m_formatter->format(logger, level, event);
        }
    }

    bool FileLogAppender::reopen()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }

    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            std::cout << m_formatter->format(logger, level, event);
        }
    }

    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }

    void LogFormatter::init()
    {
        // str, format, type
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size())
            {
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt;
            while (++n < m_pattern.size())
            {
                if (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}' && fmt_status == 0)
                {
                    break;
                }
                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        str = m_pattern.substr(i + 1, n - i - 1);
                        fmt_status = 1; // 解析格式
                        fmt_begin = n;
                        continue;
                    }
                }
                if (fmt_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        fmt_status = 2; // 结束
                        ++n;
                        break;
                    }
                }
            }

            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, "", 0));
                    nstr.clear();
                }
                str = m_pattern.substr(i + 1, n - i - 1);
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
            else if (fmt_status == 1)
            {
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
            else if (fmt_status == 2)
            {
                if (!nstr.empty())
                {
                    vec.push_back(std::make_tuple(nstr, "", 0));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            }
        }

        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string &fmt)>> s_format_items = {
#define XX(str, C)                           \
    {                                        \
        #str, [](const std::string &fmt)     \
        { return std::make_shared<C>(fmt); } \
    }

            XX(m, MessageFormatItem),
            XX(p, LevelFormatItem),
            XX(r, ElapseFormatItem),
            XX(c, NameFormatItem),
            XX(t, ThreadIdFormatItem),
            XX(n, NewLineFormatItem),
            XX(d, DataTimeFormatItem),
            XX(f, FilenameFormatItem),
            XX(l, LineFormatItem),
            XX(F, FiberIdFormatItem),

#undef XX
        };

        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                m_items.push_back(std::make_shared<StringFormatItem>(std::get<0>(i)));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    m_items.push_back(std::make_shared<StringFormatItem>("<<error_format %" + std::get<0>(i) + ">>"));
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        }
    }
}