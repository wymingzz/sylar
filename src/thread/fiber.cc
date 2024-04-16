#include <atomic>

#include "thread/fiber.h"
#include "config/config.h"
#include "tools/mcaro.h"

namespace sylar
{
    static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id{0};    // 用于创建协程id
    static std::atomic<uint64_t> s_fiber_count{0}; // 协程数量

    static thread_local Fiber *t_fiber = nullptr;           // 当前协程
    static thread_local Fiber::ptr t_threadFiber = nullptr; // 主协程

    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
        Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

    class MallocStackAllocator
    {
    public:
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }

        static void Dealloc(void *vp, size_t size)
        {
            return free(vp);
        }
    };

    using StackAllocator = MallocStackAllocator;

    Fiber::Fiber()
    {
        m_state = EXEC;
        SetThis(this);

        if (getcontext(&m_ctx))
        {
            SYLAR_ASSERT_MSG(false, "getcontext failed!");
        }

        ++s_fiber_count;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber";
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize)
        : m_id(++s_fiber_id), m_stacksize(stacksize ? stacksize : g_fiber_stack_size->getValue()), m_cb(cb)
    {
        m_stack = StackAllocator::Alloc(m_stacksize);
        if (getcontext(&m_ctx))
        {
            SYLAR_ASSERT_MSG(false, "getcontext failed!");
        }
        if (t_threadFiber == nullptr)
        {
            GetThis();
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;

        ++s_fiber_count;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
    }

    Fiber::~Fiber()
    {
        if (m_stack)
        {
            SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);

            StackAllocator::Dealloc(m_stack, m_stacksize);
        }
        else
        {
            SYLAR_ASSERT(!m_cb);
            SYLAR_ASSERT(m_state == EXEC);

            Fiber *cur = t_fiber;
            if (cur == this)
            {
                SetThis(nullptr);
            }
        }
        --s_fiber_count;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id = " << m_id;
    }

    void Fiber::reset(std::function<void()> cb)
    {
        SYLAR_ASSERT(m_stack);
        SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        m_cb = cb;
        if (getcontext(&m_ctx))
        {
            SYLAR_ASSERT_MSG(false, "getcontext failed!");
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        m_state = INIT;
    }

    void Fiber::swapIn()
    {
        SetThis(this);
        SYLAR_ASSERT(m_state != EXEC);

        m_state = EXEC;
        if (swapcontext(&t_threadFiber->m_ctx, &m_ctx))
        {
            SYLAR_ASSERT_MSG(false, "swapcontext faild!");
        }
    }

    void Fiber::swapOut()
    {
        SetThis(t_threadFiber.get());

        if (swapcontext(&m_ctx, &t_threadFiber->m_ctx))
        {
            SYLAR_ASSERT_MSG(false, "swapcontext faild!");
        }
    }

    uint64_t Fiber::GetFiberId()
    {
        if (t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }

    Fiber::ptr Fiber::GetThis()
    {
        if (t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        Fiber::ptr main_fiber(new Fiber);
        SYLAR_ASSERT(t_fiber == main_fiber.get());
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }

    void Fiber::YieldToReady()
    {
        Fiber::ptr cur = GetThis();
        if (cur == t_threadFiber)
        {
            return;
        }
        cur->m_state = READY;
        cur->swapOut();
    }

    void Fiber::YieldToHold()
    {
        Fiber::ptr cur = GetThis();
        if (cur == t_threadFiber)
        {
            return;
        }
        cur->m_state = HOLD;
        cur->swapOut();
    }

    uint64_t Fiber::TotalFilbers()
    {
        return s_fiber_count;
    }

    void Fiber::MainFunc()
    {
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur);
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch (std::exception &ex)
        {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what();
        }
        catch (...)
        {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except";
        }

        auto raw_ptr = cur.get();
        cur.reset();
        raw_ptr->swapOut();

        SYLAR_ASSERT_MSG(false, "never reach");
    }

}