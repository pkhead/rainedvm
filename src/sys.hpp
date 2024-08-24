#pragma once
#include <string>

namespace sys
{
    int subprocess(const std::string &cmdline, std::string &out_stdout);
    bool open_url(const std::string &url);
}