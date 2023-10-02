#include "log/log.h"
#include <iostream>
#include <functional>
#include "config/config.h"

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

    LogLevel::Level LogLevel::FromString(const std::string str)
    {
        std::string level = str;
        std::transform(level.begin(), level.end(), level.begin(), ::toupper);
#define XX(name)               \
    if (level == #name)        \
    {                          \
        return LogLevel::name; \
    }

        XX(DEBUG);
        XX(INFO);
        XX(WARN);
        XX(ERROR);
        XX(FATAL);
        return LogLevel::UNKNOW;

#undef XX
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {}

    LogEventWrap::~LogEventWrap()
    {
        m_event->getLogger()->log(m_event->getLovel(), m_event);
    }

    std::stringstream &LogEventWrap::getSS()
    {
        return m_event->getSS();
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,
                       const char *file, int32_t line, uint32_t elapse,
                       uint32_t thread_id, uint32_t fiber_id, uint64_t time)
        : m_file(file),
          m_line(line),
          m_elapse(elapse),
          m_threadId(thread_id),
          m_fiberId(fiber_id),
          m_time(time),
          m_logger(logger),
          m_level(level) {}

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    Logger::Logger(const std::string name) : m_name(name), m_level(LogLevel::DEBUG)
    {
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}\t%t\t%F\t[%p]\t[%c]\t%f:%l\t%m %n"));
    }

    void Logger::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(m_mutex);
        for (auto &i : m_appenders)
        {
            if (m_formatter == i->getFormatter())
            {
                i->setFormatter(val);
            }
        }
        m_formatter = val;
    }

    void Logger::setFormatter(const std::string &val)
    {
        LogFormatter::ptr new_val = std::make_shared<LogFormatter>(val);
        if (new_val->isError())
        {
            std::cout << "logger setFormatter error name = " << m_name
                      << " value = " << val << " invalid formatter" << std::endl;
            return;
        }
        setFormatter(new_val);
    }

    std::string Logger::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        node["level"] = LogLevel::ToString(m_level);
        if (!m_formatter->getPattern().empty())
        {
            node["formatter"] = m_formatter->getPattern();
        }

        for (auto &i : m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LogFormatter::ptr Logger::getFormatter()
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        if (!appender->getFormatter())
        {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppender()
    {
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }

    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            auto self = shared_from_this();
            MutexType::Lock lock(m_mutex);
            if (!m_appenders.empty())
            {
                for (auto &i : m_appenders)
                {
                    i->log(self, level, event);
                }
            }
            else if (m_root)
            {
                m_root->log(level, event);
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

    void LogAppender::setFormatter(LogFormatter::ptr val)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter = val;
    }

    void LogAppender::setFormatter(const std::string &val)
    {
        MutexType::Lock lock(m_mutex);
        LogFormatter::ptr new_val = std::make_shared<LogFormatter>(val);
        if (new_val->isError())
        {
            std::cout << "logappender setFormatter error"
                      << " value = " << val << " invalid formatter" << std::endl;
            return;
        }
        m_formatter = new_val;
    }

    LogFormatter::ptr LogAppender::getFormatter()
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    FileLogAppender::FileLogAppender(const std::string &filename)
        : m_filename(filename)
    {
        reopen();
    }

    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            MutexType::Lock lock(m_mutex);
            m_filestream << m_formatter->format(logger, level, event);
        }
    }

    std::string FileLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (!m_formatter->getPattern().empty())
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    bool FileLogAppender::reopen()
    {
        MutexType::Lock lock(m_mutex);
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
            MutexType::Lock lock(m_mutex);
            std::cout << m_formatter->format(logger, level, event);
        }
    }

    std::string StdoutLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        if (m_level != LogLevel::UNKNOW)
        {
            node["level"] = LogLevel::ToString(m_level);
        }
        if (!m_formatter->getPattern().empty())
        {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
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
                m_error = true;
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
            XX(T, TabFormatItem),

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
                    m_error = true;
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        }
    }

    LoggerManager::LoggerManager()
    {
        m_root.reset(new Logger);

        m_root->addAppender(std::make_shared<StdoutLogAppender>());

        m_loggers[m_root->m_name] = m_root;

        init();
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }

        Logger::ptr logger = std::make_shared<Logger>(name);
        logger->m_root = m_root;
        m_loggers[name] = logger;

        return logger;
    }

    struct LogAppenderDefine
    {
        int type = 0; // 1 File, 2 Stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &oth) const
        {
            return type == oth.type &&
                   level == oth.level &&
                   formatter == oth.formatter &&
                   file == oth.file;
        }
    };

    struct LogDefine
    {
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;

        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &oth) const
        {
            return name == oth.name &&
                   level == oth.level &&
                   formatter == oth.formatter &&
                   appenders == oth.appenders;
        }

        bool operator<(const LogDefine &oth) const
        {
            return name < oth.name;
        }
    };

    // template <>
    // class LexicalCast<std::string, std::set<LogDefine>>
    // {
    // public:
    //     std::set<LogDefine> operator()(const std::string &v)
    //     {
    //         YAML::Node node = YAML::Load(v);
    //         std::set<LogDefine> vec;
    //         for (size_t i = 0; i < node.size(); ++i)
    //         {
    //             auto n = node[i];
    //             if (!n["name"].IsDefined())
    //             {
    //                 std::cout << "log config  error: name is null, " << n << std::endl;
    //                 continue;
    //             }

    //             LogDefine ld;
    //             ld.name = n["name"].as<std::string>();
    //             ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
    //             if (n["formatter"].IsDefined())
    //             {
    //                 ld.formatter = n["formatter"].as<std::string>();
    //             }

    //             if (n["appenders"].IsDefined())
    //             {
    //                 for (size_t j = 0; j < n["appenders"].size(); ++j)
    //                 {
    //                     auto a = n["appenders"][j];
    //                     if (!a["type"].IsDefined())
    //                     {
    //                         std::cout << "log config  error: appender type is null, " << a << std::endl;
    //                         continue;
    //                     }
    //                     std::string type = a["type"].as<std::string>();
    //                     LogAppenderDefine lad;
    //                     if (type == "FileLogAppender")
    //                     {
    //                         lad.type = 1;
    //                         if (!a["file"].IsDefined())
    //                         {
    //                             std::cout << "log config  error: fileappender file is null, " << a << std::endl;
    //                             continue;
    //                         }
    //                         lad.file = a["file"].as<std::string>();
    //                     }
    //                     else if (type == "StdoutLogAppender")
    //                     {
    //                         lad.type = 2;
    //                     }
    //                     else
    //                     {
    //                         std::cout << "log config  error: appender type is invalid, " << a << std::endl;
    //                         continue;
    //                     }

    //                     if (a["formatter"].IsDefined())
    //                     {
    //                         lad.formatter = a["formatter"].as<std::string>();
    //                     }

    //                     ld.appenders.push_back(lad);
    //                 }
    //             }

    //             vec.insert(ld);
    //         }
    //         return vec;
    //     }
    // };

    // template <>
    // class LexicalCast<std::set<LogDefine>, std::string>
    // {
    // public:
    //     std::string operator()(const std::set<LogDefine> &v)
    //     {
    //         YAML::Node node;
    //         for (auto &i : v)
    //         {
    //             YAML::Node n;
    //             n["name"] = i.name;
    //             n["level"] = LogLevel::ToString(i.level);

    //             if (i.formatter.empty())
    //             {
    //                 n["formatter"] = i.formatter;
    //             }

    //             for (auto &a : i.appenders)
    //             {
    //                 YAML::Node na;
    //                 if (a.type == 1)
    //                 {
    //                     na["type"] = "FileLogAppender";
    //                     na["file"] = a.file;
    //                 }
    //                 else if (a.type == 2)
    //                 {
    //                     na["type"] = "StdoutLogAppender";
    //                 }

    //                 na["level"] = LogLevel::ToString(a.level);

    //                 if (!a.formatter.empty())
    //                 {
    //                     na["formatter"] = a.formatter;
    //                 }

    //                 n["appenders"].push_back(na);
    //             }
    //             node.push_back(n);
    //         }
    //         std::stringstream ss;
    //         ss << node;
    //         return ss.str();
    //     }
    // };

    template <>
    class LexicalCast<std::string, LogAppenderDefine>
    {
    public:
        LogAppenderDefine operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            LogAppenderDefine ans;
            if (!node["type"].IsDefined())
            {
                std::cout << "log config  error: appender type is null, " << node << std::endl;
                return LogAppenderDefine();
            }
            std::string type = node["type"].as<std::string>();
            if (type == "FileLogAppender")
            {
                ans.type = 1;
                if (!node["file"].IsDefined())
                {
                    std::cout << "log config  error: fileappender file is null, " << node << std::endl;
                    return LogAppenderDefine();
                }
                ans.file = node["file"].as<std::string>();
            }
            else if (type == "StdoutLogAppender")
            {
                ans.type = 2;
            }
            else
            {
                std::cout << "log config  error: appender type is invalid, " << node << std::endl;
                return LogAppenderDefine();
            }

            ans.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");

            if (node["formatter"].IsDefined())
            {
                ans.formatter = node["formatter"].as<std::string>();
            }

            return ans;
        }
    };

    template <>
    class LexicalCast<LogAppenderDefine, std::string>
    {
    public:
        std::string operator()(const LogAppenderDefine &v)
        {
            YAML::Node node;
            if (v.type == 1)
            {
                node["type"] = "FileLogAppender";
                node["file"] = v.file;
            }
            else if (v.type == 2)
            {
                node["type"] = "StdoutLogAppender";
            }
            if (v.level != LogLevel::UNKNOW)
            {
                node["level"] = LogLevel::ToString(v.level);
            }
            if (!v.formatter.empty())
            {
                node["formatter"] = v.formatter;
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    template <>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string &v)
        {
            YAML::Node node = YAML::Load(v);
            LogDefine ans;
            if (!node["name"].IsDefined())
            {
                std::cout << "log config  error: name is null, " << node << std::endl;
                return LogDefine();
            }
            ans.name = node["name"].as<std::string>();
            ans.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
            if (node["formatter"].IsDefined())
            {
                ans.formatter = node["formatter"].as<std::string>();
            }
            if (node["appenders"].IsDefined())
            {
                std::stringstream ss;
                for (size_t i = 0; i < node["appenders"].size(); ++i)
                {
                    ss.str("");
                    ss << node["appenders"][i];
                    ans.appenders.push_back(LexicalCast<std::string, LogAppenderDefine>()(ss.str()));
                }
            }
            return ans;
        }
    };

    template <>
    class LexicalCast<LogDefine, std::string>
    {
    public:
        std::string operator()(const LogDefine &v)
        {
            YAML::Node node;
            node["name"] = v.name;
            node["level"] = LogLevel::ToString(v.level);
            if (!v.formatter.empty())
            {
                node["formatter"] = v.formatter;
            }
            for (auto &a : v.appenders)
            {
                node["appenders"].push_back(LexicalCast<LogAppenderDefine, std::string>()(a));
            }

            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
        Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter
    {
        LogIniter()
        {
            g_log_defines->addListener(0xF1E231, [](const std::set<LogDefine> &old_val, const std::set<LogDefine> &new_val)
                                       {
                                            for(auto &i:new_val){
                                                Logger::ptr logger;
                                                auto it = old_val.find(i);
                                                if(it == old_val.end()){
                                                    // 新增的logger(i)
                                                    logger = SYLAR_LOG_NAME(i.name);
                                                }else{
                                                    if(!(i == *it)){
                                                        // 修改的logger(i)
                                                        logger = SYLAR_LOG_NAME(i.name);
                                                    }else{
                                                        continue;
                                                    }
                                                }

                                                logger->setLevel(i.level);
                                                if(!i.formatter.empty()){
                                                    logger->setFormatter(i.formatter);
                                                }

                                                logger->clearAppender();
                                                for(auto &a:i.appenders){
                                                    LogAppender::ptr ap;
                                                    if(a.type == 1){
                                                        ap.reset(new FileLogAppender(a.file));
                                                    }else if(a.type == 2){
                                                        ap.reset(new StdoutLogAppender);
                                                    }
                                                    ap->setLevel(a.level);
                                                    if(!a.formatter.empty()){
                                                        ap->setFormatter(a.formatter);
                                                    }
                                                    logger->addAppender(ap);
                                                }
                                            }

                                            for(auto &i:old_val){
                                                auto it = new_val.find(i);
                                                if(it == new_val.end()){
                                                    // 删除的logger(i)
                                                    auto logger = SYLAR_LOG_NAME(i.name);
                                                    logger->setLevel((LogLevel::Level)100); // 设置高级别，使得日志无法输出
                                                    logger->clearAppender();
                                                }
                                            } });
        }
    };

    static LogIniter __log__init;

    std::string LoggerManager::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for (auto &i : m_loggers)
        {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void LoggerManager::init()
    {
    }
}