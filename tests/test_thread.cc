#include "sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
int count = 0;
// sylar::RWMutex s_mutex;
sylar::Mutex s_mutex;

void fun1()
{
    SYLAR_LOG_INFO(g_logger) << "name: " << sylar::Thread::GetName()
                             << " this.name: " << sylar::Thread::GetThis()->getName()
                             << " id: " << sylar::GetThreadId()
                             << " this.id: " << sylar::Thread::GetThis()->getId();

    for (int i = 0; i < 100000; ++i)
    {
        // sylar::RWMutex::WriteLock(&s_mutex);
        sylar::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void fun2()
{
    while (true)
    {
        SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}

void fun3()
{
    while (true)
    {
        SYLAR_LOG_INFO(g_logger) << "====================================================================================================================";
    }
}

int main(int argc, char const *argv[])
{
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/wyming/Documents/C++/sylar/bin/conf/log2.yml");
    sylar::Config::LoadFromYaml(root);

    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 2; ++i)
    {
        sylar::Thread::ptr thr = std::make_shared<sylar::Thread>(&fun2, "name_" + std::to_string(i * 2));
        sylar::Thread::ptr thr2 = std::make_shared<sylar::Thread>(&fun3, "name_" + std::to_string(i * 2 + 1));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for (size_t i = 0; i < thrs.size(); ++i)
    {
        thrs[i]->join();
    }

    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count = " << count;

    return 0;
}
