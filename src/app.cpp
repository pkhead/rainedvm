#include <imgui.h>
#include <cstdio>
#include <cpr/cpr.h>
#include "app.hpp"
#include "sys.hpp"
#include "json.hpp"
#include "imgui_markdown.h"

using namespace nlohmann; // what

Application::Application()
{
    frame = 0;
    cur_state = AppState::FETCH_LIST;
    selected_version = -1;
}

Application::~Application()
{

}

// get the installed rained version by querying the executable
static bool get_rained_version(std::string &version)
{
    const char *rained_exe = std::getenv("RAINED");
    if (rained_exe == nullptr)
    {
        rained_exe = "Rained";
    }

#if _WIN32
    std::string const_cmd = std::string(rained_exe) + " --console --version";
#else
    std::string const_cmd = std::string(rained_exe) + " --version";
#endif

    std::string res;
    sys::subprocess(const_cmd, res);
    
    if (res.substr(0, 7) != "Rained ")
        return false;

    version = res.substr(7);
    return true;
}

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

static bool process_rained_versions(std::vector<ReleaseInfo> &releases)
{
    /*const char *user_agent = "Rained.Update/" RAINEDUPDATE_VERSION " (" SYS_ARCH "-" SYS_OS "-" SYS_ABI ") libcpr/" CPR_VERSION " libcurl/" LIBCURL_VERSION;
    printf("user agent: %s\n", user_agent);

    cpr::Response r = cpr::Get(
        cpr::Url("https://api.github.com/repos/pkhead/rained/releases"),
        cpr::UserAgent(user_agent)
    );

    printf("status code: %li\n", r.status_code);
    printf("content type: %s\n", r.header["content-type"].c_str());
    if (r.status_code != 200)
    {
        return false;
    }

    std::string desired_content_type = "application/json";
    if (r.header["content-type"].substr(0, desired_content_type.length()) != desired_content_type)
    {
        return false;
    }

    json result = json::parse(r.text);*/
    std::ifstream test_f("releases.json");
    json result = json::parse(test_f);
    
    if (!result.is_array())
        return false;
    
    for (auto it = result.begin(); it != result.end(); it++)
    {
        if (!it->is_object()) return false;

        ReleaseInfo release{};
        release.version_name = it->at("name");

        // don't show beta versions...
        if (release.version_name[0] == 'b') continue;

        release.api_url = it->at("url");
        release.url = it->at("html_url");

        auto assets = it->at("assets");
        if (!assets.is_array()) return false;

        for (auto asset = assets.begin(); asset != assets.end(); asset++)
        {
            std::string content_type = asset->at("content_type");
            if (content_type == "application/x-gzip")
                release.linux_download_url = asset->at("browser_download_url");
            else if (content_type == "application/x-zip-compressed")
                release.windows_download_url = asset->at("browser_download_url");
        }

        release.changelog = std::string("[View on GitHub](") + release.url + ")\n" + std::string(it->at("body"));

        releases.push_back(release);
    }

    return true;
}

static void markdown_link_callback(ImGui::MarkdownLinkCallbackData data)
{
    if (data.isImage) return;
    if (!sys::open_url(std::string(data.link, data.linkLength)))
    {
        fprintf(stderr, "could not open url");
    }
}

void Application::render_main_window()
{
    ImGui::BeginMenuBar();
    {
        if (ImGui::MenuItem("About"))
        {
            about_window_open = true;
        }
    }
    ImGui::EndMenuBar();

    // about window
    if (about_window_open)
    {
        if (ImGui::Begin("About", &about_window_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("Rained Version Manager " RAINEDUPDATE_VERSION);
            ImGui::SeparatorText("Credits");
            ImGui::BulletText("GLFW");
            ImGui::BulletText("Dear ImGui");
            ImGui::BulletText("nlohmann::json");
            ImGui::BulletText("imgui_markdown");
        } ImGui::End();
    }

    switch (cur_state)
    {
        case AppState::FETCH_LIST:
        {
            ImGui::Text("Fetching current Rained version...");

            if (frame > 10)
            {
                try
                {
                    if (get_rained_version(current_version) &&
                        process_rained_versions(available_versions))
                    {
                        cur_state = AppState::CHOOSE_VERSION;
                    }
                    else
                    {
                        cur_state = AppState::FETCH_LIST_ERROR;
                    }
                }
                catch (...)
                {
                    cur_state = AppState::FETCH_LIST_ERROR;
                }
            }

            break;
        }
        
        case AppState::FETCH_LIST_ERROR:
        {
            ImGui::Text("An error occured. Please try again later.");
            break;
        }
        
        case AppState::CHOOSE_VERSION:
        {
            ImGui::Text("Current version: %s", current_version.c_str());

            ImGuiChildFlags child_flags = ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX;
            ImGui::BeginChild("Version List", ImVec2(ImGui::GetFontSize() * 10.0f, -FLT_MIN), child_flags);
            {
                int index = 0;
                for (auto it = available_versions.begin(); it != available_versions.end(); it++)
                {
                    std::string label;
                    if (it->version_name == current_version)
                        label = it->version_name + " (current)";
                    else
                        label = it->version_name;
                        
                    if (ImGui::Selectable(label.c_str(), index == selected_version))
                        selected_version = index;

                    index++;
                }
            }
            ImGui::EndChild();

            ImGui::SameLine();
            ImGui::BeginChild("Changelog", ImGui::GetContentRegionAvail());
            if (selected_version >= 0)
            {
                ReleaseInfo &release = available_versions[selected_version];

                ImGui::MarkdownConfig md_config{};
                md_config.formatCallback = ImGui::defaultMarkdownFormatCallback;
                md_config.linkCallback = markdown_link_callback;
                ImGui::Markdown(release.changelog.c_str(), release.changelog.length(), md_config);
            }
            ImGui::EndChild();
            break;
        }
    }
    
    frame++;
}