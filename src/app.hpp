#pragma once

#include <string>
#include <vector>

struct ReleaseInfo
{
    std::string version_name;
    std::string url;
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

public:
    Application();
    ~Application();

    void render();
}; // class Application