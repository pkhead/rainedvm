#pragma once

#include <filesystem>

namespace zip
{
    class ZipExtractor
    {
    private:
        struct impl;
        impl *p_impl;

        std::filesystem::path dest_directory;
    
    public:
        ZipExtractor(const ZipExtractor&) = delete;
        ZipExtractor& operator=(ZipExtractor const&) = delete;

        ZipExtractor(std::filesystem::path zip_path, std::filesystem::path dest_directory);
        ~ZipExtractor();

        void extract();
    }; // class ZipExtractor
} // namespace zip