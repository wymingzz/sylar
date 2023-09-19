#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <stdint.h>
#include <zconf.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>

namespace sylar{

    pid_t GetThreadId();

    uint32_t GetFiberId();

}

#endif