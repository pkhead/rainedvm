#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <thread>
#include <mutex>

struct ReleaseInfo
{
    std::string version_name;
    std::string api_url;
    std::string url;
    std::string changelog;
    std::string linux_download_url;
    std::string windows_download_url;
}; // struct ReleaseInfo

class InstallTask
{
private:
    std::thread _thread;
    std::mutex _mutex;
    std::string _rained_dir;

    std::string _prog_msg;
    float _progress;
    bool _cancel_requested;

    bool _is_thread_done;

    const ReleaseInfo release;

    void _thread_proc();
    void _install();
    
public:
    InstallTask(const InstallTask&) = delete;
    InstallTask& operator=(InstallTask const&) = delete;
    InstallTask(const std::filesystem::path &rained_dir, const ReleaseInfo &release);
    ~InstallTask();

    // returns true if still processing, false if done.
    bool get_progress(std::string &out_msg, float &out_progress);
    void cancel();
}; // class InstallTask

class Application
{
private:
    enum class AppState
    {
        FETCH_LIST,
        FETCH_LIST_ERROR,
        CHOOSE_VERSION
    } cur_state;

    unsigned int frame;
    std::filesystem::path rained_dir;
    std::string current_version;
    std::vector<ReleaseInfo> available_versions;

    std::unique_ptr<InstallTask> _install_task;

    int selected_version;
    bool about_window_open = false;

    void install_version(const ReleaseInfo &release_info);

public:
    Application(const Application&) = delete;
    Application& operator=(Application const&) = delete;

    Application();
    ~Application();

    void render_main_window();
}; // class Application