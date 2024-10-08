#pragma once
#include <ostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define THROW_WIN32_ERROR() throw std::system_error(std::error_code(GetLastError(), std::system_category()))
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define SYS_ARCH "x86_64"
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#define SYS_ARCH "i386"
#else
#define SYS_ARCH "unknown"
#endif

#ifdef _WIN32
#define SYS_OS "pc-windows"
#elif defined(__APPLE__)
#define SYS_OS "apple"
#elif defined(__linux__)
#define SYS_OS "linux"
#else
#define SYS_OS "unknown"
#endif

#ifdef __GNUC__
#define SYS_ABI "gnu"
#elif defined(_MSC_VER)
#define SYS_ABI "msvc"
#else
#define SYS_ABI "unknown"
#endif

#define SYS_TRIPLET SYS_ARCH "-" SYS_OS "-" SYS_ABI

namespace sys
{
    int subprocess(const std::string &cmdline, std::ostream &stdout_stream);
    int subprocess(const std::string &cmdline);

    bool open_url(const std::string &url);

    const std::vector<std::string>& arguments();
}