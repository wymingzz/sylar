#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

#include "thread/thread.h"

namespace sylar
{

    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        typedef std::shared_ptr<Fiber> ptr;

        Fiber(std::function<void()> cb, size_t stacksize = 0);
        ~Fiber();

        // 重置协程函数，并重置状态(只有在INIT和TERM状态才会重置)
        void reset(std::function<void()> cb);
        // 切换到当前协程执行
        void swapIn();
        // 将当前协程切换到后台
        void swapOut();

        uint64_t getId() const { return m_id; }

        // 线程状态
        enum State
        {
            INIT,   // 初始化
            HOLD,   // 暂停(人为)
            EXEC,   // 正在执行
            TERM,   // 终止
            READY,  // 准备
            EXCEPT, // 执行出错
        };

        // 返回当前协程id
        static uint64_t GetFiberId();
        // 返回当前协程
        static Fiber::ptr GetThis();
        // 设置当前协程
        static void SetThis(Fiber *f);
        // 协程切换到后台，并设置为READY状态
        static void YieldToReady();
        // 协程切换到后台，并设置为HOLD状态
        static void YieldToHold();

        // 总协程数
        static uint64_t TotalFilbers();
        // 执行方法
        static void MainFunc();

    private:
        Fiber();

    private:
        uint64_t m_id = 0;        // 协程id
        uint32_t m_stacksize = 0; // 栈大小
        State m_state = INIT;     // 协程状态

        ucontext_t m_ctx;
        void *m_stack = nullptr; // 栈指针

        std::function<void()> m_cb; // 调用方法
    };

}

#endif