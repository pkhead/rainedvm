#pragma once
#include <string>

namespace sys
{
    int subprocess(const std::string &cmdline, std::string &out_stdout);
}