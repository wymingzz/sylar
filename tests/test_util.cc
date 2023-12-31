#include "sylar.h"
#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_assert()
{
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10, 2, "   ");
    SYLAR_ASSERT_MSG(0 == 1, "abcdefg");
}

int main(int argc, char const *argv[])
{
    test_assert();
    return 0;
}
