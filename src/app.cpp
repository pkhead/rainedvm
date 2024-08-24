#include <imgui.h>
#include <cstdio>
#include <cpr/cpr.h>
#include "app.hpp"
#include "sys.hpp"
#include "json.hpp"
#include "zip.hpp"
#include "imgui_markdown.h"
#include "util.hpp"

using namespace nlohmann; // what

const char *USER_AGENT = "RainedVersionManager/" RAINEDUPDATE_VERSION " (" SYS_TRIPLET ") libcpr/" CPR_VERSION " libcurl/" LIBCURL_VERSION;

Application::Application()
{
    frame = 0;
    cur_state = AppState::FETCH_LIST;
    selected_version = -1;

    // obtain rained directory from the RAINED_DIRECTORY environment variable,
    // or if not defined the current working directory.
    const char *rained_env = std::getenv("RAINED_DIRECTORY");

    if (rained_env == nullptr)
        rained_dir = std::filesystem::current_path();
    else
        rained_dir = std::filesystem::u8path(rained_env).native();
}

Application::~Application()
{

}

// get the installed rained version by querying the executable
static bool get_rained_version(const std::filesystem::path &rained_path, std::string &version)
{
    std::filesystem::path rained_exe_path = rained_path / "Rained";

#if _WIN32
    std::string const_cmd = rained_exe_path.u8string() + " --console --version";
#else
    std::string const_cmd = rained_exe_path.u8string() + " --version";
#endif

    std::string res;
    sys::subprocess(const_cmd, res);
    
    if (res.substr(0, 7) != "Rained ")
        return false;

    version = res.substr(7);

    // trim whitespace at end
    for (auto it = version.rbegin(); it != version.rend(); it++)
    {
        if (!std::isspace(*it))
        {
            version.erase(it.base(), version.end());
            break;
        }
    }

    return true;
}

static bool process_rained_versions(std::vector<ReleaseInfo> &releases)
{
    // TODO: add https://api.github.com/repos/pkhead/rained/releases/tags/nightly to the top of the list

#ifdef DEBUG
    std::ifstream test_f("releases.json");
    json result = json::parse(test_f);
#else
    cpr::Response r = cpr::Get(
        cpr::Url("https://api.github.com/repos/pkhead/rained/releases"),
        cpr::UserAgent(USER_AGENT)
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

    json result = json::parse(r.text);
#endif
    
    if (!result.is_array())
        return false;
    
    for (auto it = result.begin(); it != result.end(); it++)
    {
        if (!it->is_object()) return false;

        ReleaseInfo release{};
        release.version_name = it->at("name");

        // don't show beta versions...
        // or nightly, code manually fetches data for that
        if (release.version_name[0] == 'b') continue;
        if (it->at("tag_name") == "nightly") continue;

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
            ImGui::BulletText("minizip-ng");
            ImGui::BulletText("cpr");
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
                    if (get_rained_version(rained_dir, current_version) &&
                        process_rained_versions(available_versions))
                    {
                        // set selected version to current
                        for (unsigned int i = 0; i < available_versions.size(); i++)
                        {
                            if (available_versions[i].version_name == current_version)
                            {
                                selected_version = i;
                                break;
                            }
                        }

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
            ImGui::BeginGroup();

            if (selected_version >= 0)
            {
                ImVec2 content_region_avail = ImGui::GetContentRegionAvail();
                ImGui::BeginChild("Changelog", ImVec2(content_region_avail.x, content_region_avail.y - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.y));
                ReleaseInfo &release = available_versions[selected_version];

                ImGui::MarkdownConfig md_config{};
                md_config.formatCallback = ImGui::defaultMarkdownFormatCallback;
                md_config.linkCallback = markdown_link_callback;
                ImGui::Markdown(release.changelog.c_str(), release.changelog.length(), md_config);

                ImGui::EndChild();
                ImGui::BeginDisabled(release.version_name == current_version);
                if (ImGui::Button("Install"))
                {
                    install_version(available_versions[selected_version]);
                }
                ImGui::EndDisabled();
            }

            ImGui::EndGroup();
            break;
        }
    }
    
    frame++;
}

void Application::install_version(const ReleaseInfo &release)
{
    // install zip
    // TODO: uncomment download code when path is working...
    std::filesystem::path temp_path = std::filesystem::temp_directory_path() / ("rainedvm-rained-" + release.version_name);
    std::string download_url;

#if defined(_WIN32)
    if (strcmp(SYS_ARCH, "x86_64") == 0)
        download_url = release.windows_download_url;
#elif defined(__linux__)
    if (strcmp(SYS_ARCH, "x86_64") == 0)
        download_url = release.linux_download_url;
#elif defined(__APPLE__)
    if (strcmp(SYS_ARCH, "x86_64") == 0)
        download_url = release.macos_download_url;
#endif

    if (download_url.empty())
    {
        throw std::runtime_error("ERROR: no " SYS_OS  " download for " + release.version_name);
        return;
    }

    if (!std::filesystem::exists(temp_path))
        std::filesystem::create_directory(temp_path);

#ifdef _WIN32
    std::filesystem::path download_archive_path = temp_path / "download.zip";
#elif defined(__linux__)
    std::filesystem::path download_archive_path = temp_path / "download.tar.gz";
#else
    #error No version downloader for this platform
#endif

    // download archive
    if (!std::filesystem::exists(download_archive_path))
    {
        std::ofstream ar_of(download_archive_path, std::ios::binary);
        cpr::Response r = cpr::Download(ar_of,
            cpr::Url(download_url),
            cpr::UserAgent(USER_AGENT)
        );

        if (r.status_code != 200)
        {
            throw std::runtime_error("ERROR: http download status code is " + std::to_string(r.status_code));
            return;
        }
    }

    std::filesystem::path install_path = temp_path / "extracted";
    if (!std::filesystem::exists(install_path))
        std::filesystem::create_directory(install_path);

    // zip extract
#ifdef _WIN32
    {
        zip::ZipExtractor zip_reader(temp_path / "download.zip", install_path);
        zip_reader.extract();
    }
#elif defined(__linux__)
    {
        std::string gzip_cmd = "gzip -dk \"" + download_archive_path.u8string() + "\"";
        std::string tar_cmd = "tar -C \"" + install_path.u8string() + "\" -xf \"" + (temp_path / "download.tar").u8string() + "\"";

        std::string _;
        if (sys::subprocess(gzip_cmd, _) != 0)
        {
            throw std::runtime_error(util::format("command failed: %s\n", gzip_cmd.c_str()));
            return;
        }

        if (sys::subprocess(tar_cmd, _) != 0)
        {
            std::filesystem::remove(temp_path / "download.tar");
            throw std::runtime_error(util::format("command failed: %s\n", tar_cmd.c_str()));
            return;
        }

        std::filesystem::remove(temp_path / "download.tar");
    }
#else
    #error "No version extractor for this platform"
#endif

    std::filesystem::remove_all(install_path);
    printf("%s\n", temp_path.c_str());
}