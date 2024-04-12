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

constexpr const char* capnp_top_level_attributes_node_name = "file_attributes";
constexpr const char* capnp_root_node_name = "data";
constexpr const char* capnp_data_node_name = "data";
constexpr const char* capnp_dim_node_name = "dims";

#include <fmt/format.h>
#include <string_view>

namespace uda::plugins::netcdf {

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

    Buffer read_data(const std::string& data_path);

    void handle_atomic_variable(netCDF::Variable& variable, NodeBuilder* node, TreeBuilder* tree);

    inline netCDF::Variable get_variable(const std::string& request) {
        return get_variable(_file, request);
    };

    netCDF::Group get_subtree(const std::string& request);

    void walk(netCDF::Group& group, NodeBuilder* node, TreeBuilder* tree);

    void store_all_nc_maps(netCDF::Group& group);

    [[nodiscard]] inline std::vector<std::string> get_variables() const {
        std::vector<std::string> result;
        result.reserve(_variables.size());
        for (const auto& variable_pair: _variables) {
            result.emplace_back(variable_pair.first);
        }
        return result;
    }

    [[nodiscard]] inline std::vector<std::string> get_groups() const {
        std::vector<std::string> result;
        result.reserve(_groups.size());
        for (const auto& variable_pair: _groups) {
            result.emplace_back(variable_pair.first);
        }
        return result;
    }

    [[nodiscard]] inline std::vector<std::string> get_coordinates() const {
        std::vector<std::string> result;
        result.reserve(_coordinates.size());
        for (const auto& variable_pair: _coordinates) {
            result.emplace_back(variable_pair.first);
        }
        return result;
    }

    [[nodiscard]] inline std::vector<std::string> get_index_dimensions() const {
        std::vector<std::string> result;
        result.reserve(_index_dimensions.size());
        for (const auto& variable_pair: _index_dimensions) {
            result.emplace_back(variable_pair.first);
        }
        return result;
    }

    [[nodiscard]] std::vector<std::string> get_attributes() const;

    static bool request_is_variable(const netCDF::Group& input_node, const std::string& request);

    static inline bool request_string_is_valid_nc_path(const std::string& request) {
        const std::regex r(R"(^(\/[0-9a-zA-Z_]+)+(\.[0-9a-zA-Z_]+)?$|^\/(\.[0-9a-zA-Z_]+)?$)");
        return std::regex_match(request, r);
    }

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



