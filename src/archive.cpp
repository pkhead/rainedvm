#include <mz.h>
#include <mz_zip.h>
#include <mz_strm.h>
#include <mz_strm_os.h>
#include <mz_zip_rw.h>
#include <chrono>
#include <sstream>
#include "archive.hpp"
#include "sys.hpp"
#include "util.hpp"

archive::basic_archive::basic_archive(const std::filesystem::path &archive_path) :
    archive_path(archive_path)
{}

//////////////////
// .zip archive //
//////////////////

struct archive::zip_archive::impl
{
    void *zip_reader;
    std::vector<std::filesystem::path> entries;
}; // archive::ZipArchive::impl

archive::zip_archive::zip_archive(const std::filesystem::path &zip_path) :
    basic_archive(zip_path),
    p_impl(new impl)
{
    p_impl->zip_reader = mz_zip_reader_create();
    if (mz_zip_reader_open_file(p_impl->zip_reader, zip_path.u8string().c_str()) != MZ_OK)
        throw archive::archive_exception("could not open zip archive");

    if (mz_zip_reader_goto_first_entry(p_impl->zip_reader) != MZ_OK)
        throw archive::archive_exception("could not read archive directory");

    std::vector<std::filesystem::path> entries;
    
    while (true)
    {
        mz_zip_file *file_info = nullptr;
        if (mz_zip_reader_entry_get_info(p_impl->zip_reader, &file_info) != MZ_OK)
            throw archive::archive_exception("could not read archive directory");

        const char *entry_path = file_info->filename;
        printf("%s\n", entry_path);

        int err = mz_zip_reader_goto_next_entry(p_impl->zip_reader);
        if (err == MZ_END_OF_LIST) break;
        else if (err != MZ_OK)
            throw archive::archive_exception("could not read archive directory");
    }

    p_impl->entries = std::move(entries);
}

archive::zip_archive::~zip_archive()
{
    mz_zip_reader_close(p_impl->zip_reader);
    mz_zip_reader_delete(&p_impl->zip_reader);
    delete p_impl;
}

void archive::zip_archive::extract_all(const std::filesystem::path &dest_dir)
{
    mz_zip_reader_save_all(p_impl->zip_reader, dest_dir.u8string().c_str());
}

void archive::zip_archive::extract_file(const std::filesystem::path &entry_path, const std::filesystem::path &dest_dir)
{
    if (mz_zip_reader_locate_entry(p_impl->zip_reader, entry_path.c_str(), false) != MZ_OK)
        throw archive::archive_exception("could not locate entry");

    std::filesystem::path dest_path = dest_dir / entry_path.filename();

    if (mz_zip_reader_entry_save_file(p_impl->zip_reader, dest_path.u8string().c_str()) != MZ_OK)
        throw archive::archive_exception("could not write to file");
}

const std::vector<std::filesystem::path>& archive::zip_archive::files()
{
    return p_impl->entries;
}






/////////////////////
// .tar.gz archive //
/////////////////////
struct archive::tar_archive::impl
{
    bool is_temporary;
    std::vector<std::filesystem::path> entries;
}; // struct archive::tar_archive::impl

archive::tar_archive::tar_archive(const std::filesystem::path &tar_path, bool is_temporary) :
    basic_archive(tar_path),
    p_impl(new impl)
{
    p_impl->is_temporary = is_temporary;

    std::stringstream dir_list;
    if (sys::subprocess(util::format("tar -tf \"%s\"", tar_path.u8string().c_str()), dir_list) != 0)
        throw archive::archive_exception("could not read archive directory");

    std::string line;
    while (std::getline(dir_list, line))
    {
        p_impl->entries.push_back(line);
    }
}

archive::tar_archive::~tar_archive()
{
    if (p_impl->is_temporary)
        std::filesystem::remove(archive_path);
    delete p_impl;
}

archive::tar_archive::tar_archive(const std::filesystem::path &tar_path) :
    tar_archive(tar_path, false)
{}

archive::tar_archive archive::tar_archive::from_gzipped(std::filesystem::path tar_gz_path)
{
    unsigned long ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::string suffix = "-" + std::to_string(ms);

    std::string filename = tar_gz_path.filename().u8string() + suffix + ".tar";

    std::filesystem::path tar_path = std::filesystem::temp_directory_path() / filename;

    std::string cmd = util::format("gzip -dk \"%s\"", tar_gz_path.u8string().c_str());
    if (sys::subprocess(cmd.c_str()) != 0)
        throw std::runtime_error("failed to run command: " + cmd);

    std::filesystem::path in_tar_path = tar_gz_path.replace_extension(); // removed gz extension
    std::filesystem::rename(in_tar_path, tar_path);

    return tar_archive(tar_path, true);
}

const std::vector<std::filesystem::path>& archive::tar_archive::files()
{
    return p_impl->entries;
}

void archive::tar_archive::extract_file(const std::filesystem::path &entry_path, const std::filesystem::path &dest_dir)
{
    std::string cmd = util::format("tar -C \"%s\" -xf \"%s\" \"%s\"", dest_dir.u8string().c_str(), archive_path.u8string().c_str(), entry_path.u8string().c_str());
    if (sys::subprocess(cmd) != 0)
        throw archive::archive_exception("failed to extract file");
}

void archive::tar_archive::extract_all(const std::filesystem::path &dest_dir)
{
    std::string cmd = util::format("tar -C \"%s\" -xf \"%s\"", dest_dir.u8string().c_str(), archive_path.u8string().c_str());
    if (sys::subprocess(cmd) != 0)
        throw archive::archive_exception("failed to extract archive");
}