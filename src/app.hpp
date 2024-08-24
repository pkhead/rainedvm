#pragma once

#include <string>
#include <vector>

struct ReleaseInfo
{
    std::string version_name;
    std::string api_url;
    std::string url;
    std::string changelog;
    std::string linux_download_url;
    std::string windows_download_url;
};

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
    std::string current_version;
    std::vector<ReleaseInfo> available_versions;

    int selected_version;
    bool about_window_open = false;

public:
    Application();
    ~Application();

    void render_main_window();
}; // class Application