#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <stdint.h>
#include <zconf.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <vector>
#include <string>

namespace sylar
{

    pid_t GetThreadId();

    uint32_t GetFiberId();

    // 获取函数栈内容
    void Backtrace(std::vector<std::string> &bt, int size, int skip = 1);
    std::string BacktraceToString(int size, int skip = 2, const std::string &prefix = "");

}

#endif