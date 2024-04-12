#pragma once

#include "netcdfpp.h"
// #include <netcdf.h>
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <tuple>
#include <set>
#include <regex>

#include "serialisation/capnp_serialisation.h"

// mangle the index label a bit so it's unlikely to clash with a real dimension label
// alternatively can store as a different data type in capnp node.
#define INDEX_ONLY_DIMENSION_LABEL  "__uda_nc_index_only_dimension__"
#define CAPNP_TOP_LEVEL_ATTRIBUTES_NODE_NAME "file_attributes"
#define CAPNP_REQUEST_NODE_NAME "request"
#define CAPNP_DATA_NODE_NAME "data"
#define CAPNP_DIM_NODE_NAME "dims"
#define CAPNP_DATTRIBUTE_NODE_NAME "attributes"

#include <fmt/format.h>
#include <string_view>

namespace uda::plugins::netcdf {

// struct CapnpTreeNode
// {
//     TreeBuilder* tree;
//     NodeBuilder* node;
// };

// struct Data
// {
//     char* data;
//     size_t size;
//     std::string type;
// };

// struct DimBlock
// {
//     char* data;
//     int dim_n;
//     std::string label;
//     std::string units;
//     std::string type;
// };

// struct DataBlock
// {
//     char* data;
//     int data_n;
//     int rank;
//     std::vector<DimBlock> dims;
//     std::string label;
//     std::string units;
//     std::string type;
// };

enum class RequestType {
    INVALID_PATH,
    GROUP,
    VARIABLE,
    GROUP_ATTRIBUTE,
    VARIABLE_ATTRIBUTE
};


class CoordinateNotFoundError : public std::exception {
public:
    explicit CoordinateNotFoundError(const std::string& coordinate_name) : _coordinate_name(coordinate_name) {
        _message = fmt::format("Co-ordinate name \"{}\" not found", _coordinate_name);
    };

    [[nodiscard]] const char* what() const noexcept override {
        return _message.c_str();
    };
private:
    std::string _coordinate_name;
    std::string _message;
};


class Reader {
public:
    // constructors
    explicit Reader(const std::string& filename);

    inline ~Reader() { _file.close(); }

    // utils
    static std::vector<std::string> split(const std::string& s, const std::string& delimiter);

    static std::string get_full_path(const netCDF::Group& input_group);

    static std::string get_full_path(const netCDF::Variable& input_variable);

    std::pair<bool, Buffer> handle_request(const std::string& request);

    void handle_atomic_variable(netCDF::Variable& variable, NodeBuilder* node, TreeBuilder* tree);

    inline netCDF::Variable get_variable(const std::string& request) {
        return get_variable(_file, request);
    };

    netCDF::Group get_subtree(const std::string& request);

    void walk(netCDF::Group& group, NodeBuilder* node, TreeBuilder* tree);

    void store_all_nc_maps(netCDF::Group& group);

    inline std::vector<std::string> get_variables() const {
        std::vector<std::string> result;
        for (const auto& variable_pair: _variables) {
            result.emplace_back(variable_pair.first);
        }
        return result;
    }

    inline std::vector<std::string> get_groups() const {
        std::vector<std::string> result;
        for (const auto& variable_pair: _groups) {
            result.emplace_back(variable_pair.first);
        }
        return result;
    }

    inline std::vector<std::string> get_coordinates() const {
        std::vector<std::string> result;
        for (const auto& variable_pair: _coordinates) {
            result.emplace_back(variable_pair.first);
        }
        return result;
    }

    inline std::vector<std::string> get_index_dimensions() const {
        std::vector<std::string> result;
        for (const auto& variable_pair: _index_dimensions) {
            result.emplace_back(variable_pair.first);
        }
        return result;
    }

    std::vector<std::string> get_attributes() const;

    static bool request_is_variable(const netCDF::Group& input_node, const std::string& request);

    inline bool request_is_variable(const std::string& request) {
        return request_is_variable(_file, request);
    }

    static bool request_is_structured_data(const netCDF::Group& input_node, const std::string& request);

    inline bool request_is_structured_data(const std::string& request) {
        return request_is_structured_data(_file, request);
    }

    inline bool request_is_group(const std::string& request) const {
        return _groups.find(request) != _groups.end();
    }

    inline bool request_string_is_valid_nc_path(const std::string& request) {
        const std::regex r("^(\\/[0-9a-zA-Z_]+)+(\\.[0-9a-zA-Z_]+)?$|^\\/(\\.[0-9a-zA-Z_]+)?$");
        return std::regex_match(request, r);
        // return true;
    }

    bool request_is_attribute(const std::string& request);
    // std::vector<std::string> get_attributes();

    RequestType check_request_path(const std::string& request);

    void print_ids(netCDF::Group& group);

    std::unordered_map<std::string, netCDF::Variable> get_all_variables(netCDF::Group& group);

    std::unordered_map<std::string, netCDF::Group> get_all_groups(netCDF::Group& group);

    netCDF::Variable find_coordinate(const std::unordered_map<std::string, netCDF::Variable>& variables, std::string name);

    // accessors
    inline bool file_is_open() { return _file.is_open(); }

private:
    netCDF::File _file;
    std::unordered_map<std::string, netCDF::Group> _groups;
    std::unordered_map<std::string, netCDF::Variable> _variables;
    std::unordered_map<std::string, netCDF::Variable> _coordinates;
    std::unordered_map<std::string, netCDF::Dimension> _index_dimensions;
    std::unordered_map<int, netCDF::UserType> _usertypes;

    static netCDF::Variable get_variable(const netCDF::Group& input_node, const std::string& request);

    netCDF::Group get_subtree(const netCDF::Group& input_node, const std::string& request);

    std::set<std::string> _request_coords = {};

    void handle_user_type_variable(netCDF::Variable& variable, NodeBuilder* node, TreeBuilder* tree);

    void walk_compound_type(netCDF::UserType& var, char* bytes_data_array, NodeBuilder* node, TreeBuilder* tree);

    void add_nc_coordinate_data(NodeBuilder* node, TreeBuilder* tree);

    std::pair<std::string, std::string> split_attribute_path(const std::string& request);
};
}



