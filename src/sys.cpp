#include <system_error>
#include "sys.hpp"

#ifdef _WIN32
#include <windows.h>
#define THROW_WIN32_ERROR() throw std::system_error(std::error_code(GetLastError(), std::system_category()))
#endif

int sys::subprocess(const std::string &cmdline, std::string &out_stdout)
{
#ifdef _WIN32
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    HANDLE cout_r = NULL;
    HANDLE cout_w = NULL;
    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = true;

    if (!CreatePipe(&cout_r, &cout_w, &sa, 0))
        THROW_WIN32_ERROR();

    if (!SetHandleInformation(cout_r, HANDLE_FLAG_INHERIT, 0))
        THROW_WIN32_ERROR();
    
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = cout_w;
    si.hStdError = cout_w;
    si.wShowWindow = SW_HIDE;

    //si.dwFlags = STARTF_USESHOWWINDOW;
    //si.wShowWindow = SW_HIDE;

    char cmd[256];
    strcpy_s(cmd, 256, cmdline.c_str());

    BOOL success = CreateProcess(
        NULL,
        cmd,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (success)
    {
        CloseHandle(pi.hThread);
        CloseHandle(cout_w);
    }
    else
    {
        THROW_WIN32_ERROR();
    }
    
    CHAR buf[128];
    ZeroMemory(buf, sizeof(buf));
    DWORD dwRead;

    while (ReadFile(cout_r, buf, sizeof(buf)-1, &dwRead, NULL))
        out_stdout.append(buf, dwRead);

    if (GetLastError() != ERROR_BROKEN_PIPE)
        THROW_WIN32_ERROR();
    
    CloseHandle(cout_r);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    
    return (int)exitCode;
#else
    char buf[128];

    auto pipe = popen(cmdline.c_str(), "r");
    if (!pipe)
        return false;
    
    while (!feof(pipe))
    {
        if (fgets(buf, 128, pipe) != nullptr)
            out_stdout += buf;
    }

    return pclose(pipe);
#endif
}