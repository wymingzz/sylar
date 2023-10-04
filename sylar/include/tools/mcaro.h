#ifndef __SYLAR_MCARO_H__
#define __SYLAR_MCARO_H__

#include <string.h>
#include <assert.h>
#include "tools/util.h"

#define SYLAR_ASSERT(exp)                                                              \
    if (!(exp))                                                                        \
    {                                                                                  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #exp                        \
                                          << "\nbacktrace:\n"                          \
                                          << sylar::BacktraceToString(100, 2, "    "); \
        assert(exp);                                                                   \
    }

#define SYLAR_ASSERT_MSG(exp, msg)                                                     \
    if (!(exp))                                                                        \
    {                                                                                  \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION: " #exp                        \
                                          << "\n"                                      \
                                          << msg                                       \
                                          << "\nbacktrace:\n"                          \
                                          << sylar::BacktraceToString(100, 2, "    "); \
        assert(exp);                                                                   \
    }

#endif