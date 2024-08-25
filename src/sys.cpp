#include <system_error>
#include <stdio.h>
#include "sys.hpp"
#include "sys_args_internal.hpp"

#ifdef _WIN32
#include <windows.h>
#define THROW_WIN32_ERROR() throw std::system_error(std::error_code(GetLastError(), std::system_category()))
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

int sys::subprocess(const std::string &cmdline, std::ostream &stdout_stream)
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
    char buf[1024];

    auto pipe = popen(cmdline.c_str(), "r");
    if (!pipe)
        return -1;
    
    size_t count;
    while (!feof(pipe))
    {
        if ((count = fread(buf, 1, sizeof(buf), pipe)) > 0)
            stdout_stream.write(buf, count);
    }

    return pclose(pipe);
#endif
}

int sys::subprocess(const std::string &cmdline)
{
#ifdef _WIN32
    #error sys::subprocess(const std::string& cmdline) is not defined for Windows
#else
    auto pipe = popen(cmdline.c_str(), "r");
    if (!pipe)
        return false;

    return pclose(pipe);
#endif
}

bool sys::open_url(const std::string &url)
{
#ifdef _WIN32
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    pid_t pid;

    if ((pid = fork()) < 0)
    {
        fprintf(stderr, "fork failed");
        return false;
    }
    
    if (pid == 0)
    {
        // child process
        std::string arg0 = "xdg-open";
        std::string arg1 = url;

        char *args[3] = { arg0.data(), arg1.data(), nullptr };
        execvp(args[0], args);
        exit(0);
    }

    return true;
#endif
}

static std::vector<std::string> _args;

const std::vector<std::string>& sys::arguments()
{
    return _args;
}

void sys::set_arguments(int argc, char **argv)
{
    _args.resize(argc);
    for (int i = 0; i < argc; i++)
    {
        _args[i] = argv[i];
    }
}