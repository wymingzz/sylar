#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <time.h>

namespace sylar
{

    class Logger;

    // 日志级别
    class LogLevel
    {
    public:
        enum Level
        {
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5,
        };

        static const char *ToString(LogLevel::Level level);
    };

    // 日志事件
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time);

        const char *getFile() const { return m_file; }
        uint32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getTreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint32_t getTime() const { return m_time; }
        std::string getContent() const { return m_ss.str(); }
        std::stringstream &getSS() { return m_ss; }

    private:
        const char *m_file = nullptr; // 文件名
        int32_t m_line = 0;           // 行号
        uint32_t m_elapse = 0;        // 程序启动开始到现在的毫秒数
        uint32_t m_threadId = 0;      // 线程id
        uint32_t m_fiberId = 0;       // 协程id
        uint64_t m_time;              // 时间戳
        std::stringstream m_ss;
    };

    // 日志格式器
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        LogFormatter(const std::string &pattern);

        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem() {}
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        void init();

    private:
        std::string m_pattern;                // format结构
        std::vector<FormatItem::ptr> m_items; //
    };

    // 日志输出地
    class LogAppender
    {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender() {}

        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0; // 纯虚函数

        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        LogFormatter::ptr getFormatter() const { return m_formatter; }

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG; // 针对哪个级别的日志
        LogFormatter::ptr m_formatter;             // 输出日志格式
    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
    public:
        typedef std::shared_ptr<Logger> ptr;

        Logger(const std::string name = "root");

        void log(LogLevel::Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);

        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

        const std::string &getName() const { return m_name; }

    private:
        std::string m_name;                      // 日志名称
        LogLevel::Level m_level;                 // 日志级别
        std::list<LogAppender::ptr> m_appenders; // Appender集合
        LogFormatter::ptr m_formatter;
    };

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };

    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << logger->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getTreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };

    class DataTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DataTimeFormatItem(const std::string format = "%Y:%m:%d %H:%M:%S") : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);

            os << buf;
        }

    private:
        std::string m_format;
    };

    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str) : m_str(str) {}
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << m_str;
        }

    private:
        std::string m_str;
    };

    // class TabFormatItem : public LogFormatter::FormatItem
    // {
    // public:
    //     TabFormatItem(const std::string &str = "") {}
    //     void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
    //     {
    //         os << "\t";
    //     }
    // };

    // 输出到控制台的Appender
    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

    private:
    };

    // 输出到文件的Appender
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        FileLogAppender(const std::string &filename);

        void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

        // 重新打开文件， 文件打开成功返回true
        bool reopen();

    private:
        std::string m_filename;     // 输出文件名称
        std::ofstream m_filestream; // 打开的文件流
    };

}

#endif