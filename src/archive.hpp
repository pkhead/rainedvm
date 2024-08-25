#pragma once

#include <filesystem>
#include <vector>

namespace archive
{
    class archive_exception : public std::runtime_error
    {
    public:
        archive_exception(const std::string &msg) : std::runtime_error(msg)
        {}
    }; // class archive_exception
    
    /**
    * Base for a class that represents an archive format (.zip, .tar.gz)
    */
    class basic_archive
    {
    protected:
        std::filesystem::path archive_path;
        basic_archive(const std::filesystem::path &archive_path);
    
    public:
        basic_archive(const basic_archive&) = delete;
        basic_archive(basic_archive&&);
        basic_archive& operator=(basic_archive const&) = delete;
        virtual ~basic_archive() {}

        /**
        * Get a list of all files in the archive.
        **/
        virtual const std::vector<std::filesystem::path>& files() = 0;

        /**
        * Extract a file from the archive.
        **/
        virtual void extract_file(const std::filesystem::path &entry_path, const std::filesystem::path &dest_dir) = 0;

        /**
        * Extract all files from the archive into a destination directory.
        **/
        virtual void extract_all(const std::filesystem::path &dest_dir) = 0;
    }; // class archive

    /**
    * Represents a .zip file.
    **/
    class zip_archive : public basic_archive
    {
    private:
        struct impl;
        impl *p_impl;
    
    public:
        zip_archive(const std::filesystem::path &zip_path);
        ~zip_archive() override;

        const std::vector<std::filesystem::path>& files() override;
        void extract_file(const std::filesystem::path &entry_path, const std::filesystem::path &dest_dir) override;
        void extract_all(const std::filesystem::path &dest_dir) override;
    }; // class zip_archive

    /**
    * Represents a .tar file.
    **/
    class tar_archive : public basic_archive
    {
    private:
        struct impl;
        impl *p_impl;

        tar_archive(const std::filesystem::path &tar_path, bool is_temporary);
    
    public:
        tar_archive(tar_archive&&);
        tar_archive(const std::filesystem::path &tar_path);
        ~tar_archive() override;

        static tar_archive from_gzipped(std::filesystem::path tar_gz_path);

        const std::vector<std::filesystem::path>& files() override;
        void extract_file(const std::filesystem::path &entry_path, const std::filesystem::path &dest_dir) override;
        void extract_all(const std::filesystem::path &dest_dir) override;
    }; // class gzip_archive
} // namespace archive