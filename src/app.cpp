#include <imgui.h>
#include <cstdio>
#include <sstream>
#include <cassert>
#include <cpr/cpr.h>
#include <unordered_set>
#include <condition_variable>
#include "app.hpp"
#include "sys.hpp"
#include "json.hpp"
#include "archive.hpp"
#include "imgui_markdown.h"
#include "util.hpp"

using namespace nlohmann; // what

const char *USER_AGENT = "RainedVersionManager/" RAINEDUPDATE_VERSION " (" SYS_TRIPLET ") libcpr/" CPR_VERSION " libcurl/" LIBCURL_VERSION;

Application::Application()
{
    frame = 0;
    cur_state = AppState::FETCH_LIST;
    selected_version = -1;
    is_rained_installed = true; // will check that it isn't later
    cur_release_info = {};

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

    std::stringstream p_res;
    if (sys::subprocess(const_cmd, p_res) != 0)
        return false;
    
    std::string res = p_res.str();

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
    printf("update version list\n");
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
            if (content_type == "application/x-gzip" || content_type == "application/gzip")
                release.linux_download_url = asset->at("browser_download_url");
            else if (content_type == "application/x-zip-compressed" || content_type == "application/zip")
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

bool Application::query_current_version()
{
    is_rained_installed = true;

    // rained not existing is a valid state...
    if (!std::filesystem::is_regular_file(rained_dir / "Rained") && !std::filesystem::is_regular_file(rained_dir / "Rained.exe"))
    {
        is_rained_installed = false;
        current_version.clear();
    }
    else if (!get_rained_version(rained_dir, current_version))
    {
        return false;
    }

    if (!available_versions.empty() || process_rained_versions(available_versions))
    {
        // set selected version to current
        for (unsigned int i = 0; i < available_versions.size(); i++)
        {
            if (available_versions[i].version_name == current_version)
            {
                selected_version = i;
                cur_release_info = available_versions[i];
                break;
            }
        }

        if (is_rained_installed && cur_release_info.url.empty())
            throw std::runtime_error("could not find current release info...");

        cur_state = AppState::CHOOSE_VERSION;
    }
    else
    {
        return false;
    }
    
    return true;
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
                bool success;

                try { success = query_current_version(); }
                catch (...) { success = false; }

                if (!success)
                    cur_state = AppState::FETCH_LIST_ERROR;
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
            if (is_rained_installed)
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

    if (_install_task)
    {
        std::string prog_msg;
        float progress_value;
        bool is_done = !_install_task->get_progress(prog_msg, progress_value);
        bool is_faulted = _install_task->get_exception(prog_msg);

        if (is_done && !is_faulted)
        {
            _install_task = nullptr;

            bool s;
            try { s = query_current_version(); }
            catch (...) { s = false; }

            if (!s)
            {
                fprintf(stderr, "error fetching current version");
                current_version.clear();
            }
        }
        else
        {
            if (!ImGui::IsPopupOpen("Installing..."))
                ImGui::OpenPopup("Installing...");

            if (ImGui::BeginPopupModal("Installing...", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                float max_width = ImGui::GetFontSize() * 30.0f;

                ImGui::PushTextWrapPos(max_width);

                if (!is_faulted)
                {
                    ImGui::TextWrapped("%s", prog_msg.c_str());

                    if (progress_value >= 0.0f)
                    {
                        ImGui::ProgressBar(progress_value, ImVec2(max_width, 0.0f));
                    }
                    else // negative progress value means it should display an indeterminate value
                    {
                        ImGui::ProgressBar(-1.0f * ImGui::GetTime(), ImVec2(max_width, 0.0f));
                    }

                    if (ImGui::Button("Cancel"))
                        _install_task->cancel();
                }
                else
                {
                    ImGui::TextWrapped("ERROR! %s", prog_msg.c_str());
                    if (ImGui::Button("OK"))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }

                // handle any overwrite prompts
                OverwritePromptInfo prompt_info;
                if (_install_task->get_overwrite_prompt(prompt_info))
                {
                    if (!ImGui::IsPopupOpen("Overwrite?"))
                        ImGui::OpenPopup("Overwrite?");

                    if (ImGui::BeginPopupModal("Overwrite?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        ImGui::Text("Local changes were detected in %s.", prompt_info.display_file_path.c_str());
                        ImGui::Separator();
                        if (ImGui::Button("Overwrite Changes"))
                        {
                            _install_task->set_overwrite_prompt_result(1);
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Keep Changes"))
                        {
                            _install_task->set_overwrite_prompt_result(0);
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Cancel"))
                        {
                            _install_task->set_overwrite_prompt_result(2);
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }
                }
                ImGui::EndPopup();
            }
        }
    }
    
    frame++;
}








///////////////////////
// INSTALLATION TASK //
///////////////////////

InstallTask::InstallTask(const std::filesystem::path &rained_dir, const ReleaseInfo &cur_release, const ReleaseInfo &desired_release) :
    _rained_dir(rained_dir),
    cur_release(cur_release),
    desired_release(desired_release)
{
    _progress = 0;
    _cancel_requested = false;
    _is_thread_done = false;
    _thread_faulted = false;

    _thread = std::thread(&InstallTask::_thread_proc, this);
}

bool InstallTask::get_progress(std::string &out_msg, float &out_progress)
{
    std::lock_guard guard(_mutex);
    if (_is_thread_done) return false;

    out_msg = _prog_msg;
    out_progress = _progress;

    return true;
}

bool InstallTask::get_exception(std::string &out_except)
{
    std::lock_guard guard(_mutex);
    if (_is_thread_done && _thread_faulted)
    {
        out_except = _thread_exception;
        return true;
    }

    return false;
}

void InstallTask::cancel()
{
    std::lock_guard guard(_mutex);
    _cancel_requested = true;
}

bool InstallTask::get_overwrite_prompt(OverwritePromptInfo &out_prompt_info) const
{
    if (_overwrite_prompt)
    {
        out_prompt_info.display_file_path = _overwrite_prompt->display_file_path;
        out_prompt_info.cv = _overwrite_prompt->cv;
        return true;
    }
    else
    {
        return false;
    }
}

void InstallTask::set_overwrite_prompt_result(int condition)
{
    std::lock_guard lock(_mutex);
    if (_overwrite_prompt)
    {
        _overwrite_prompt->user_input = condition;
        _overwrite_prompt->cv->notify_all();
    }
}

int InstallTask::prompt_overwrite(const std::filesystem::path &file_path)
{
    {
        std::unique_ptr<OverwritePromptInfo> info = std::make_unique<OverwritePromptInfo>();
        info->display_file_path = file_path.u8string(); // file_path is already relative here
        info->user_input = -1;
        info->cv = std::make_shared<std::condition_variable>();

        auto lock = std::lock_guard(_mutex);
        _overwrite_prompt = std::move(info);
    }

    std::unique_lock lock(_mutex);
    _overwrite_prompt->cv->wait(lock, [&]{ return this->_overwrite_prompt->user_input != -1; });
    
    if (!lock.owns_lock())
        lock.lock();
    int ret = _overwrite_prompt->user_input;
    _overwrite_prompt = nullptr;
    lock.unlock();

    return ret;
}

void InstallTask::_thread_proc()
{
    try
    {
        _install();

        auto guard = std::lock_guard(_mutex);
        _is_thread_done = true;
    }
    catch (std::runtime_error e)
    {
        auto guard = std::lock_guard(_mutex);
        _thread_faulted = true;
        _thread_exception = e.what();
        _is_thread_done = true;
    }
}

static const std::filesystem::path rainedvm_path()
{
    static bool need_init = true;
    static std::filesystem::path path;

    if (need_init)
    {
        need_init = false;

        path = std::filesystem::path(".rainedvm");
        if (!std::filesystem::exists(path))
            std::filesystem::create_directory(path);
    }

    return path;
}

static std::filesystem::path download_release(const ReleaseInfo &release, std::function<bool(float)> progress_callback)
{
    std::filesystem::path root_dir = rainedvm_path();

    // install zip
    std::filesystem::path archive_path = root_dir / std::filesystem::path("rained-" + release.version_name);
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
        throw std::runtime_error("ERROR: no " SYS_OS  " download for " + release.version_name);

#ifdef _WIN32
    std::filesystem::path download_archive_path = root_dir / std::filesystem::path("rained-" + release.version_name + ".zip");
#elif defined(__linux__)
    std::filesystem::path download_archive_path = root_dir / std::filesystem::path("rained-" + release.version_name + ".tar.gz");
#else
    #error No version downloader for this platform
#endif

    // download archive
    bool canceled = false;

    if (!std::filesystem::exists(download_archive_path))
    {
        if (!progress_callback(0.0f)) return download_archive_path;

        std::ofstream ar_of(download_archive_path, std::ios::binary);
        cpr::Response r = cpr::Download(ar_of,
            cpr::Url(download_url),
            cpr::UserAgent(USER_AGENT),
            cpr::ProgressCallback([&](cpr::cpr_off_t download_total, cpr::cpr_off_t download_now, cpr::cpr_off_t, cpr::cpr_off_t, intptr_t)
            {
                canceled = !progress_callback((float)download_now / download_total);
                return !canceled;
            })
        );

        if (r.status_code != 200)
            throw std::runtime_error("ERROR: http download status code is " + std::to_string(r.status_code));
    }

    if (canceled)
    {
        std::filesystem::remove(download_archive_path);
        return "";
    }

    return download_archive_path;
}

static std::unique_ptr<archive::basic_archive> read_archive_for_platform(std::filesystem::path archive_path)
{
    #ifdef _WIN32
    archive::basic_archive *archive = new archive::zip_archive(archive_path);
    #elif defined(__linux__)
    archive::basic_archive *archive = new archive::tar_archive(archive::tar_archive::from_gzipped(archive_path));
    #else
    #error archive type for current platform is undefined
    #endif

    return std::unique_ptr<archive::basic_archive>(archive);
}

void InstallTask::_install()
{
    std::filesystem::path rvm_path = rainedvm_path();
    std::string prog_msg;

    auto progress_callback = [&](float prog)
    {
        std::lock_guard guard(_mutex);
        _prog_msg = prog_msg;
        _progress = prog;

        return !_cancel_requested;
    };

    // install zip
    std::filesystem::path cur_release_archive;
    if (!cur_release.url.empty())
    {
        prog_msg = "Fetching current version...";
        cur_release_archive = download_release(cur_release, progress_callback);
        if (_cancel_requested) return; // too lazy to lock mutex
    }

    prog_msg = util::format("Fetching %s...", desired_release.version_name.c_str());
    std::filesystem::path new_release_archive = download_release(desired_release, progress_callback);
    if (_cancel_requested) return; // to lazy to lock mutex

    std::unordered_set<std::string> ignore_list;

    if (!cur_release_archive.empty())
    {
        {
            std::lock_guard guard(_mutex);
            _prog_msg = "Removing old version...";
            _progress = -1.0f;
        }

        auto ar_for_cur = read_archive_for_platform(cur_release_archive);

        // remove all files that were downloaded for this version
        // TODO: if the user tampered with the file, ask if they want it overwritten
        std::unordered_set<std::filesystem::path> directories;

        std::filesystem::path vm_exe_path = std::filesystem::path(sys::arguments()[0]).filename();

        auto &files = ar_for_cur->files();
        for (auto &path : files)
        {
            // don't delete the version manager executable or config/imgui.ini
            // and additionally, make sure that they aren't replaced when installing
            bool ignore_file = path == vm_exe_path || path == "config/imgui.ini";

            std::filesystem::path file_dest_path = _rained_dir / path;

            // don't perform any checks in the drizzle cast folder
            // it's not named drizzle rn, but i do wish i named it that...
            // future-proofing it
            std::string path_str = path.u8string();
            if (!ignore_file &&
                path_str.substr(0, 15) != "assets/internal" &&
                path_str.substr(0, 14) != "assets/drizzle" &&
                std::filesystem::is_regular_file(file_dest_path))
            {
                bool is_different = false;

                std::ifstream file_data(file_dest_path, std::ios::binary);
                if (file_data.is_open())
                {
                    std::stringstream orig_data;
                    ar_for_cur->extract_file(path, orig_data);

                    char buf0[1024];
                    char buf1[1024];
                    int buf0_read = 0;
                    int buf1_read = 0;
                    while (true)
                    {
                        file_data.read(buf0, sizeof(buf0));
                        buf0_read = file_data.gcount();

                        orig_data.read(buf1, sizeof(buf1));
                        buf1_read = file_data.gcount();

                        if (file_data.eof() != orig_data.eof() || buf0_read != buf1_read)
                        {
                            is_different = true;
                            break;
                        }

                        assert(buf0_read == buf1_read);
                        if (memcmp(buf0, buf1, buf0_read) != 0)
                        {
                            is_different = true;
                            break;
                        }

                        if (file_data.eof() || orig_data.eof())
                            break;
                    }
                }
                else
                {
                    printf("could not open %s", file_dest_path.u8string().c_str());
                }

                if (is_different)
                {
                    printf("changed: %s\n", path.u8string().c_str());
                    int overwrite_result = prompt_overwrite(path);
                    if (overwrite_result == 2) // canceled
                        return;
                    ignore_file = overwrite_result == 0;
                }
            }

            if (ignore_file)
            {
                if (std::filesystem::exists(file_dest_path))
                    ignore_list.emplace(path);
            }
            else
            {
                std::filesystem::remove(file_dest_path);

                if (path.has_parent_path())
                    directories.emplace(path.parent_path());
            }
        }

        // remove empty directories
        for (auto &dir_path : directories)
        {
            std::filesystem::path abs_path = _rained_dir / dir_path;
            if (std::filesystem::is_directory(abs_path) && std::filesystem::is_empty(abs_path))
                std::filesystem::remove(abs_path);
        }
    }

    {
        std::lock_guard guard(_mutex);
        if (_cancel_requested) return; // hmm... seems like a bad idea to cancel here
        _prog_msg = util::format("Installing %s...", desired_release.version_name.c_str());
        _progress = 0.0f;
    }
    
    auto ar_for_new = read_archive_for_platform(new_release_archive);

    // extract files for the new version
    auto &files = ar_for_new->files();
    int files_processed = 0;
    for (auto &path : files)
    {
        if (ignore_list.find(path) != ignore_list.end()) continue;
        ar_for_new->extract_file(path, _rained_dir);

        files_processed++;

        std::lock_guard guard(_mutex);
        _progress = (float)files_processed / files.size();
        if (_cancel_requested) return; // hmm... seems like a bad idea to cancel here
    }
}

InstallTask::~InstallTask()
{
    if (_thread.joinable())
        _thread.join();
}

void Application::install_version(const ReleaseInfo &release)
{
    _install_task = std::make_unique<InstallTask>(rained_dir, cur_release_info, release);
}