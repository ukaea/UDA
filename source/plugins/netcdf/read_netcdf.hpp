#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <netcdf.h>

#include "file_handle.hpp"

namespace uda::plugins::netcdf {

// ---------------------------------------------------------------------------
// Structured tree representation of the contents of a netCDF4 file.
// This is deliberately independent of the UDA data types.
// ---------------------------------------------------------------------------

/**
    * Flat (row-major) array of values read from a variable or attribute.
    *
    * - std::monostate           : no data / unsupported type (compound, vlen, opaque)
    * - std::string              : NC_CHAR data (flat buffer of all characters)
    * - std::vector<std::string> : NC_STRING data
    * - std::vector<T>           : numeric data (enum types are read as their base type)
    */
using Value = std::variant<std::monostate, std::string, std::vector<std::string>, std::vector<int8_t>,
                            std::vector<uint8_t>, std::vector<int16_t>, std::vector<uint16_t>, std::vector<int32_t>,
                            std::vector<uint32_t>, std::vector<int64_t>, std::vector<uint64_t>, std::vector<float>,
                            std::vector<double>>;

struct Dimension {
    std::string name;
    size_t size = 0;
    bool unlimited = false;
};

struct Attribute {
    std::string name;
    nc_type type = NC_NAT;
    std::string type_name;
    size_t length = 0;
    Value value;
};

struct Variable {
    std::string name;
    nc_type type = NC_NAT;
    std::string type_name;
    std::vector<std::string> dimension_names;
    std::vector<size_t> shape;
    std::vector<Attribute> attributes;
    Value data;
};

struct Group {
    std::string name;
    std::vector<Dimension> dimensions;
    std::vector<Attribute> attributes;
    std::vector<Variable> variables;
    std::vector<std::unique_ptr<Group>> groups;
};

/**
 * Result of reading a path from a file: a whole group (including the root group),
 * a single variable (with its attributes), or a single attribute.
 */
using Node = std::variant<Group, Variable, Attribute>;

Node read_netcdf_tree(const FileHandle& file_path, const std::string& data_path);

} // namespace uda::plugins::netcdf
