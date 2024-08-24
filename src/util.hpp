#pragma once
#include <string>
#include <cstdarg>

#if __GNUC__
#define PRINTF_FORMAT(start, end) __attribute__((__format__(__printf__, start, end)))
#else
#define PRINTF_FORMAT(start, end)
#endif

namespace util
{
    /**
    * sprintf into std::string
    **/
    PRINTF_FORMAT(1, 2)
    std::string format(const char *fmt, ...);

    /**
    * vsprintf into std::string
    **/
    std::string vformat(const char *fmt, va_list va);
}

#undef PRINTF_FORMAT