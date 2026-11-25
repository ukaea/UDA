#pragma once

#include <stdexcept>
#include <netcdf.h>

namespace uda::plugins::netcdf {

inline void check(int status, const std::string& context) {
    if (status != NC_NOERR) {
        throw std::runtime_error(context + ": " + nc_strerror(status));
    }
}

} // namespace uda::plugins::netcdf
