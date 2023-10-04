#include "sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
    sylar::Fiber::YieldToHold();
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
}

void test_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "main begin -1";
    {
        SYLAR_LOG_INFO(g_logger) << "main begin";
        sylar::Fiber::ptr fiber = std::make_shared<sylar::Fiber>(run_in_fiber);
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        SYLAR_LOG_INFO(g_logger) << "main end";
    }
    SYLAR_LOG_INFO(g_logger) << "main end2";
}

int main(int argc, char const *argv[])
{
    sylar::Thread::SetName("main");
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 3; ++i)
    {
        thrs.push_back(sylar::Thread::ptr(new sylar::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for (auto i : thrs)
    {
        i->join();
    }
    return 0;
}

/*
main begin
run_in_fiber begin
main after swapIn
run_in_fiber end
main end
*/
