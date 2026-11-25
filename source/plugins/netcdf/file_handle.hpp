#pragma once

#include <string>

#include "nc_check.hpp"

namespace uda::plugins::netcdf {

class FileHandle {
public:
    explicit FileHandle(const std::string& path) : path_(path) {
        check(nc_open(path.c_str(), NC_NOWRITE, &ncid_), "failed to open netCDF file '" + path + "'");
    }
    ~FileHandle() { nc_close(ncid_); }
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    [[nodiscard]] int ncid() const { return ncid_; }
    [[nodiscard]] std::string path() const { return path_; }

private:
    int ncid_ = -1;
    std::string path_;
};

} // namespace uda::plugins::netcdf
