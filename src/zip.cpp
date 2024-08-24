#include <mz.h>
#include <mz_zip.h>
#include <mz_strm.h>
#include <mz_strm_os.h>
#include <mz_zip_rw.h>
#include "zip.hpp"

struct zip::ZipExtractor::impl
{
    void *zip_reader;
}; // struct zip::ZipExtractor::impl

zip::ZipExtractor::ZipExtractor(std::filesystem::path zip_path, std::filesystem::path dest_directory) :
    p_impl(new impl),
    dest_directory(dest_directory)
{
    p_impl->zip_reader = mz_zip_reader_create();
    if (mz_zip_reader_open_file(p_impl->zip_reader, zip_path.u8string().c_str()) == MZ_OK)
    {
        printf("zip reader was opened\n");
    }
    else
    {
        throw std::runtime_error("could not open zip reader");
    }
}

void zip::ZipExtractor::extract()
{
    mz_zip_reader_save_all(p_impl->zip_reader, dest_directory.u8string().c_str());
}

zip::ZipExtractor::~ZipExtractor()
{
    mz_zip_reader_close(p_impl->zip_reader);
    mz_zip_reader_delete(&p_impl->zip_reader);
    delete p_impl;
}