#include "netcdf_reader.hpp"
#include <exception>
// #include <netcdfpp.h>
// // #include <netcdf.h>
#include <cstring>
#include <sstream>
#include <boost/algorithm/string.hpp>

namespace uda::plugins::netcdf {

std::vector<std::string> Reader::split(const std::string& string, const std::string& delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    // size_t n = std::count(s.begin(), s.end(), delimiter);
    // vector<std::string> result(n + 1);
    std::vector<std::string> result;
    std::string token;

    while ((pos_end = string.find(delimiter, pos_start)) != std::string::npos) {
        token = string.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        result.emplace_back(token);
    }

    result.emplace_back(string.substr(pos_start));
    result.erase(result.begin());

    return result;
}

std::pair<std::string, std::string> Reader::split_attribute_path(const std::string& request) {
    auto pos = request.rfind('.');
    auto parent_path = request.substr(0, pos);
    auto grp_itr = _groups.find(parent_path);

    if (grp_itr == _groups.end()) {
        std::string err = "Unknown error locating containing group: " + parent_path;
        throw std::runtime_error(err.c_str());
    }

    auto attribute_name = request.substr(pos + 1);
    return std::make_pair(parent_path, attribute_name);
}


std::string Reader::get_full_path(const netCDF::Group& input_group) {
    netCDF::Group group = input_group;

    std::vector<std::string> elements;
    while (group.parent()) {
        elements.emplace_back(group.name());
        group = group.parent().require();
    }

    std::string path = "/" + boost::join(elements, "/");

    return path;
}


std::string Reader::get_full_path(const netCDF::Variable& input_variable) {
    netCDF::Group group = input_variable.parent();

    std::vector<std::string> elements;
    while (group.parent()) {
        elements.emplace_back(group.name());
        group = group.parent().require();
    }

    std::string path = "/" + boost::join(elements, "/");

    return path;
}

// std::string get_coordinate_path(const netCDF::Group & file, )


Reader::Reader(const std::string& filename) {
    _file.open(filename, 'r');
    // _groups = get_all_groups(_file);
    // _variables = get_all_variables(_file);
    store_all_nc_maps(_file);
    for (const auto& group_path_pair: _groups) {
        for (const auto& dimension: group_path_pair.second.dimensions()) {
            try {
                auto coordinate = find_coordinate(_variables, dimension.name());
                _coordinates.insert({dimension.name(), coordinate});
            }
            catch (const CoordinateNotFoundError& e) {
                // dimension has no associated coordinate data
                _index_dimensions.insert({dimension.name(), dimension});
            }
        }
    }
}


void Reader::store_all_nc_maps(netCDF::Group& group) {
    // std::ofstream myfile("/home/uda/test_servers/test-capnp-netcdf/etc/store_maps.txt", std::ios::app);
    std::ostringstream ss;
    ss << "store_maps group: " << group.full_path() << std::endl;

    auto variables = group.variables();

    ss << "variables: " << variables.size() << std::endl;

    for (const auto& variable: variables) {
        _variables.insert({variable.full_path(), variable});
    }

    auto usertypes = group.user_types();
    ss << "usertypes: " << usertypes.size() << std::endl;
    int i = 0;
    for (auto& type_definition: usertypes) {
        _usertypes.insert({(int) type_definition.id(), type_definition});
        ss << i++ << ":" << std::endl;
        ss << "\tname: " << type_definition.name() << std::endl;
        ss << "\tid: " << type_definition.id() << std::endl;
        ss << "\tbase type: " << type_definition.basetype() << std::endl;
        ss << "\tnc type: " << type_definition.typeclass() << std::endl;
        ss << "\tmembers count: " << type_definition.memberscount() << std::endl;
        ss << "\tfields count: " << type_definition.fieldscount() << std::endl;
        ss << "\tbytes size: " << type_definition.bytes_size() << std::endl;
        int j = 0;
        for (const auto& f: type_definition.compound_fields()) {
            ss << "\tfield " << j++ << ":" << std::endl;
            ss << "\t\tfield name: " << f.name << std::endl;
            ss << "\t\ttype: " << (int) f.type << std::endl;
        }
    }

    if (group.parent()) {
        _groups.insert({group.full_path(), group});
    } else {
        _groups.insert({"/", group});
    }
    auto groups = group.groups();
    ss << "subgroups: " << groups.size() << std::endl;
    // myfile << ss.str();
    // myfile.close();
    for (auto& sub_group: groups) {
        store_all_nc_maps(sub_group);
    }
}

// can use the "maybe" construct to get both the bool and the underlying object. can then try variable and
bool Reader::request_is_variable(const netCDF::Group& input_node, const std::string& request) {
    std::vector<std::string> path_segments = split(request, std::string("/"));
    netCDF::Group current_group = input_node;
    for (const auto& segment: path_segments) {
        if (segment != path_segments.back()) {
            try {
                current_group = current_group.group(segment).require();
            }
            catch (const netCDF::Exception& e) {
                std::cout << "ERROR: invalid path, search stopped at token " << segment << "\n";
                std::cout << e.what() << std::endl;
                throw e;
            }
        }
    }
    return current_group.variable(path_segments.back());
}

bool Reader::request_is_structured_data(const netCDF::Group& input_node, const std::string& request) {
    std::vector<std::string> path_segments = split(request, std::string("/"));
    netCDF::Group current_group = input_node;
    for (const auto& segment: path_segments) {
        if (segment != path_segments.back()) {
            try {
                current_group = current_group.group(segment).require();
            }
            catch (const netCDF::Exception& e) {
                std::cout << "ERROR: invalid path, search stopped at token " << segment << "\n";
                std::cout << e.what() << std::endl;
                throw e;
            }
        }
    }
    return !current_group.variable(path_segments.back()) or
           current_group.variable(path_segments.back()).require().user_type();
}


netCDF::Variable Reader::get_variable(const netCDF::Group& input_node, const std::string& request) {
    std::vector<std::string> path_segments = split(request, std::string("/"));
    netCDF::Group current_group = input_node;
    for (const auto& segment: path_segments) {
        if (segment != path_segments.back()) {
            try {
                current_group = current_group.group(segment).require();
            }
            catch (const netCDF::Exception& e) {
                std::cout << "ERROR: invalid path, search stopped at token " << segment << "\n";
                std::cout << e.what() << std::endl;
                throw e;
            }
        }
    }
    try {
        return current_group.variable(path_segments.back()).require();
    }
    catch (const netCDF::Exception& e) {
        std::cout << "ERROR: invalid path, cannot find variable " << path_segments.back() << "\n";
        std::cout << e.what() << std::endl;
        throw e;
    }
}


netCDF::Group Reader::get_subtree(const netCDF::Group& input_node, const std::string& request) {
    std::vector<std::string> path_segments = split(request, std::string("/"));
    // if ( request == "")
    // {
    //     std::cout << "whole file request: segments length is " << path_segments.size() << std::endl;
    // }
    netCDF::Group current_group = input_node;
    for (const auto& segment: path_segments) {
        try {
            current_group = current_group.group(segment).require();
        }
        catch (const netCDF::Exception& e) {
            std::cout << "ERROR: invalid path, search stopped at token " << segment << "\n";
            std::cout << e.what() << std::endl;
            throw e;
        }
    }
    return current_group;
}

netCDF::Group Reader::get_subtree(const std::string& request) {
    try {
        return _groups.at(request);
    }
    catch (const std::exception& e) {
        std::cout << "subtree path not found" << std::endl;
        std::cerr << e.what() << '\n';
        throw (e);
    }

}

bool Reader::request_is_attribute(const std::string& request) {
    // will there be dot syntax for attributes? e.g. /path/to/group.attribute_name?

    auto pos = request.rfind('.');
    if (pos == std::string::npos) {
        pos = request.rfind("/");
    }
    if (pos == std::string::npos) {
        // path error of some kind
        std::cout << "invalid request path" << std::endl;
        return false;
    }
    std::string trunk = request.substr(0, pos);
    auto attribute_name = request.substr(pos + 1);
    auto group_it = _groups.find(trunk);
    if (group_it != _groups.end()) {
        // std::cout << "group " << trunk << " has no attribute named " << attribute_name << std::endl;
        return group_it->second.attribute(attribute_name);
    }

    auto var_it = _variables.find(trunk);
    if (var_it != _variables.end()) {
        // std::cout << "variable " << trunk << " has no attribute named " << attribute_name << std::endl;
        return var_it->second.attribute(attribute_name);
    }

    std::cout << "request subpath does not exist: " << std::endl;
    return false;
}

RequestType Reader::check_request_path(const std::string& request) {
    // how are top-level attributes queried? "/.attribute_name" ? or "/attribute_name"

    // assuming attributes always use dot syntax then, after popping off anything after the last "/"
    // in the request string, the sub-request must be a NC group (if it exists)
    if (request == "/") return RequestType::GROUP;

    if (!request_string_is_valid_nc_path(request)) {
        // std::cout << "failed regex: " << request << std::endl;
        return RequestType::INVALID_PATH;
    }

    auto pos = request.rfind('/');
    std::string trunk_group_path = request.substr(0, pos);
    if (pos == 0) {
        trunk_group_path = "/";
    }

    auto group_itr = _groups.find(trunk_group_path);
    if (group_itr == _groups.end()) {
        return RequestType::INVALID_PATH;
    }

    auto trunk_group = group_itr->second;

    auto dot_pos = request.rfind('.');
    bool attribute_syntax = dot_pos != std::string::npos;

    // special case for attributes where number of "/" is only one (top-level metadata)
    // cannot assume second level of nesting as below
    if (pos == 0 and attribute_syntax) {
        std::string att_name = request.substr(dot_pos + 1);
        auto maybe_att = trunk_group.attribute(att_name);

        if (maybe_att) return RequestType::GROUP_ATTRIBUTE;
        else return RequestType::INVALID_PATH;
    }

    auto name = request.substr(pos + 1);
    if (attribute_syntax) {
        name = request.substr(pos + 1, dot_pos - pos - 1);
    }

    auto maybe_group = trunk_group.group(name);
    if (maybe_group) {
        if (!attribute_syntax) return RequestType::GROUP;
        std::string att_name = request.substr(dot_pos + 1);
        auto maybe_att = maybe_group.require().attribute(att_name);

        if (maybe_att) return RequestType::GROUP_ATTRIBUTE;
        else return RequestType::INVALID_PATH;
    }

    auto maybe_var = trunk_group.variable(name);
    if (maybe_var) {
        if (!attribute_syntax) return RequestType::VARIABLE;
        std::string att_name = request.substr(dot_pos + 1);
        auto maybe_att = maybe_var.require().attribute(att_name);

        if (maybe_att) return RequestType::VARIABLE_ATTRIBUTE;
        else return RequestType::INVALID_PATH;
    }

    return RequestType::INVALID_PATH;
}

/*
    For testing and debugging only
*/
std::vector<std::string> Reader::get_attributes() const {
    std::vector<std::string> result;
    for (const auto& [group_path, group]: _groups) {
        for (const auto& att: group.attributes()) {
            std::string att_path = group_path + "." + att.name();
            result.emplace_back(att_path);
        }
    }
    for (const auto& [var_path, variable]: _variables) {
        for (const auto& att: variable.attributes()) {
            std::string att_path = var_path + "." + att.name();
            result.emplace_back(att_path);
        }
    }
    return result;
}

void add_nc_attribute(netCDF::Attribute& attribute, NodeBuilder* node) {
    uda_capnp_set_node_name(node, attribute.name().c_str());
    size_t data_len = attribute.size();

    switch (attribute.type()) {
        case NC_SHORT:
            uda_capnp_add_array_i16(node, attribute.get<int16_t>().data(), data_len);
            break;
        case NC_INT:
            uda_capnp_add_array_i32(node, attribute.get<int32_t>().data(), data_len);
            break;
        case NC_INT64:
            uda_capnp_add_array_i64(node, (int64_t*) attribute.get<long long>().data(), data_len);
            break;
        case NC_FLOAT:
            uda_capnp_add_array_f32(node, attribute.get<float>().data(), data_len);
            break;
        case NC_DOUBLE:
            uda_capnp_add_array_f64(node, attribute.get<double>().data(), data_len);
            break;
        case NC_CHAR:
            uda_capnp_add_array_char(node, attribute.get_string().data(), data_len);
            break;
        case NC_STRING:
            uda_capnp_add_array_char(node, attribute.get_string().data(), data_len);
            break;
        case NC_USHORT:
            uda_capnp_add_array_u16(node, attribute.get<uint16_t>().data(), data_len);
            break;
        case NC_UINT:
            uda_capnp_add_array_u32(node, attribute.get<uint32_t>().data(), data_len);
            break;
        case NC_UINT64:
            uda_capnp_add_array_u64(node, (uint64_t*) attribute.get<unsigned long long>().data(), data_len);
            break;
        default:
            throw std::runtime_error("unknown type");
    }
}

void add_nc_variable(netCDF::Variable& variable, NodeBuilder* node) {
    size_t n_children = 0;
    if (variable.user_type()) {
        auto compound_fields = variable.user_type().require().compound_fields();
        n_children = compound_fields.size();
        uda_capnp_add_children(node, n_children);
        // add_usertype_variable()
    } else {
        auto attributes = variable.attributes();
        n_children = attributes.size();
        uda_capnp_add_children(node, n_children);
    }
}

void Reader::walk(netCDF::Group& group, NodeBuilder* node, TreeBuilder* tree) {
    // uda_capnp_set_node_name(node, group.name().c_str());

    auto nc_variables = group.variables();
    auto nc_groups = group.groups();
    auto nc_attributes = group.attributes();

    size_t n_children = nc_variables.size() + nc_groups.size() + nc_attributes.size();
    if (n_children > 0) uda_capnp_add_children(node, n_children);

    for (size_t i = 0; i < nc_variables.size(); i++) {
        auto var_capnp_node = uda_capnp_get_child(tree, node, i);
        auto var_name = nc_variables[i].name().c_str();
        uda_capnp_set_node_name(var_capnp_node, var_name);
        if (nc_variables[i].user_type()) {
            handle_user_type_variable(nc_variables[i], var_capnp_node, tree);
        } else {
            handle_atomic_variable(nc_variables[i], var_capnp_node, tree);
        }
    }

    for (size_t i = nc_variables.size(), j = 0; i < nc_variables.size() + nc_attributes.size(); i++, j++) {
        auto att_capnp_node = uda_capnp_get_child(tree, node, i);
        add_nc_attribute(nc_attributes[j], att_capnp_node);
    }

    for (size_t i = nc_variables.size() + nc_attributes.size(), j = 0; i < n_children; i++, j++) {
        auto group_capnp_node = uda_capnp_get_child(tree, node, i);
        uda_capnp_set_node_name(group_capnp_node, nc_groups[j].name().c_str());
        walk(nc_groups[j], group_capnp_node, tree);
    }

}


std::unordered_map<std::string, netCDF::Group> Reader::get_all_groups(netCDF::Group& group) {
    std::unordered_map<std::string, netCDF::Group> result;
    std::string path = get_full_path(group);

    result.insert({path, group});

    for (auto& sub_group: group.groups()) {
        auto sub_group_map = get_all_groups(sub_group);
        for (const auto& group_path_pair: sub_group_map) {
            result.insert({group_path_pair.first, group_path_pair.second});
        }
    }
    return result;
}

std::unordered_map<std::string, netCDF::Variable> Reader::get_all_variables(netCDF::Group& group) {
    std::unordered_map<std::string, netCDF::Variable> result;
    std::string path = get_full_path(group);
    if (path == "/") path = "";

    for (const auto& variable: group.variables()) {
        result.insert({path + "/" + variable.name(), variable});
    }

    for (auto& sub_group: group.groups()) {
        auto sub_group_map = get_all_variables(sub_group);
        for (const auto& variable_path_pair: sub_group_map) {
            result.insert({variable_path_pair.first, variable_path_pair.second});
        }
    }
    return result;
}

netCDF::Variable
Reader::find_coordinate(const std::unordered_map<std::string, netCDF::Variable>& variables, std::string name) {
    for (const auto& [path, var]: variables) {
        if (var.name() == name and var.dimension_count() == 1 and var.dimensions()[0].name() == name) {
            return var;
        }
    }
    throw CoordinateNotFoundError(name);
}


void Reader::print_ids(netCDF::Group& group) {
    std::cout << group.full_path() + ": " << group.id() << std::endl;
    for (auto& variable: group.variables()) {
        std::cout << variable.full_path() + ": " << variable.id() << std::endl;
    }
    for (auto& subgroup: group.groups()) {
        print_ids(subgroup);
    }
}

void unpack_compound_field(netCDF::UserType::CompoundField& compound_field, NodeBuilder* node, const char* data) {
    uda_capnp_set_node_name(node, compound_field.name.c_str());
    size_t offset = compound_field.offset;

    if (compound_field.dimensions.empty()) {
        size_t data_len = 0;
        if (compound_field.type == NC_STRING) {
            std::string temp(*(char**) &data[offset]);
            data_len = temp.size();
        }
        switch (compound_field.type) {
            case NC_SHORT:
                uda_capnp_add_i16(node, *((int16_t*) &data[offset]));
                break;
            case NC_INT:
                uda_capnp_add_i32(node, *((int32_t*) &data[offset]));
                break;
            case NC_INT64:
                uda_capnp_add_i64(node, *((int64_t*) &data[offset]));
                break;
            case NC_FLOAT:
                uda_capnp_add_f32(node, *((float*) &data[offset]));
                break;
            case NC_DOUBLE:
                uda_capnp_add_f64(node, *((double*) &data[offset]));
                break;
            case NC_CHAR:
                uda_capnp_add_i8(node, *((int8_t*) &data[offset]));
                break;
            case NC_STRING:
                uda_capnp_add_array_char(node, *(char**) &data[offset], data_len);
                break;
            case NC_USHORT:
                uda_capnp_add_u16(node, *((uint16_t*) &data[offset]));
                break;
            case NC_UINT:
                uda_capnp_add_u32(node, *((uint32_t*) &data[offset]));
                break;
            case NC_UINT64:
                uda_capnp_add_u64(node, *((uint64_t*) &data[offset]));
                break;
            default:
                throw std::runtime_error("unknown type");
        }
    } else {
        //  cast data shape to vector of size_t vals from vector of ints
        std::vector<size_t> data_shape(compound_field.dimensions.begin(), compound_field.dimensions.end());
        size_t data_len = 1;
        for (const auto& elem: compound_field.dimensions) data_len *= elem;

        if (compound_field.type == NC_STRING) {
            // data_len *= 256;
            // data_shape.emplace_back(256);
            data_len = 0;
            // char* ptr = &data[offset];
            // netCDF::UserType::CompoundField* next_field = &compound_field + 1; //sizeof(netCDF::UserType::CompoundField);
            // data_len = next_field->offset - compound_field.offset - 1;
            // for (unsigned int i=0; i<data_shape[0]; ++i)
            // {
            //     data_len += strlen(&data[offset + data_len]) + 1;
            // }
            // data_shape.emplace_back((int)(data_len / data_shape[0]) + 1);
            // std::vector<std::string> blah;
            // char** ptr = (char**) &data[offset];
            // // while (ptr < &data[offset] + data_len)
            // // {
            // //     auto entry = std::string(ptr);
            // //     if (!entry.empty())
            // //     {
            // //         blah.emplace_back(entry);
            // //         ptr += strlen(ptr);
            // //     }
            // //     else ptr++;
            // // }
            // for (unsigned int i=0; i<data_shape[0]; ++i)
            // {
            //     blah.emplace_back(ptr[i]);
            //     data_len += strlen(ptr[i]) + 1;
            // }
            // data_shape.emplace_back((int)(data_len / data_shape[0]) + 1);
            // std::cout << "array string: [";
            // for (const auto& val: blah) std::cout << val << ",";
            // std::cout << "]" << std::endl;
            // std::cout << "array length: " << data_shape[0] << std::endl;
            // std::cout << "estimated total length: " << data_len <<std::endl;
            // std::cout << "buffer length: " << data_shape[0] * data_shape[1] <<std::endl;
        }

        switch (compound_field.type) {
            case NC_SHORT:
                uda_capnp_add_md_array_i16(node, (int16_t*) &data[offset], data_shape.data(), data_shape.size());
                break;
            case NC_INT:
                uda_capnp_add_md_array_i32(node, (int32_t*) &data[offset], data_shape.data(), data_shape.size());
                break;
            case NC_INT64:
                uda_capnp_add_md_array_i64(node, (int64_t*) &data[offset], data_shape.data(), data_shape.size());
                break;
            case NC_FLOAT:
                uda_capnp_add_md_array_f32(node, (float*) &data[offset], data_shape.data(), data_shape.size());
                break;
            case NC_DOUBLE:
                uda_capnp_add_md_array_f64(node, (double*) &data[offset], data_shape.data(), data_shape.size());
                break;
            case NC_CHAR:
                uda_capnp_add_md_array_i8(node, (int8_t*) &data[offset], data_shape.data(), data_shape.size());
                break;
            case NC_STRING: {
                char** str_array = (char**) &data[offset];
                size_t string_array_len = data_shape[0];
                for (unsigned int i = 0; i < string_array_len; ++i) {
                    char* this_c_string = str_array[i];
                    data_len += strlen(this_c_string) + 1;
                }
                std::vector<char> string_array(data_len);
                size_t string_offset = 0;
                for (unsigned int i = 0; i < string_array_len; ++i) {
                    char* this_c_string = str_array[i];
                    size_t this_c_string_len = strlen(this_c_string) + 1;
                    void* buffer_dest = (void*) (string_array.data() + string_offset);
                    memcpy(buffer_dest, (void*) this_c_string, this_c_string_len);
                    string_offset += this_c_string_len;
                }
                data_shape.emplace_back((int) (data_len / data_shape[0]) + 1);
                uda_capnp_add_md_array_char(node, (char*) string_array.data(), data_shape.data(), data_shape.size());
                break;
            }
            case NC_USHORT:
                uda_capnp_add_md_array_u16(node, (uint16_t*) &data[offset], data_shape.data(), data_shape.size());
                break;
            case NC_UINT:
                uda_capnp_add_md_array_u32(node, (uint32_t*) &data[offset], data_shape.data(), data_shape.size());
                break;
            case NC_UINT64:
                uda_capnp_add_md_array_u64(node, (uint64_t*) &data[offset], data_shape.data(), data_shape.size());
                break;
            default:
                throw std::runtime_error("unknown type");
        }
    }
}

void Reader::handle_user_type_variable(netCDF::Variable& variable, NodeBuilder* node, TreeBuilder* tree) {
    // uda_capnp_set_node_name(node, variable.name().c_str());

    auto usertype_var = variable.user_type().require();
    auto compound_fields = usertype_var.compound_fields();
    // auto enum_members = usertype_var.enum_members();

    size_t compound_obj_size = usertype_var.bytes_size();
    std::vector<char> bytes_data_array(compound_obj_size);
    // netcdfpp forbids read<char*> -- have to cast to void
    variable.read<void>(bytes_data_array.data());

    size_t n_children = compound_fields.size();
    uda_capnp_add_children(node, n_children);

    walk_compound_type(usertype_var, bytes_data_array.data(), node, tree);
    // for(size_t i=0; i<n_children; ++i)
    // {
    //     // for array strings could be useful to pass n and i somehow
    //     // //auto child_node = uda_capnp_get_child(tree, node, i);
    //     // //unpack_compound_field(compound_fields[i], child_node, bytes_data_array);
    //
    // }
}

void Reader::walk_compound_type(netCDF::UserType& var, char* bytes_data_array, NodeBuilder* node, TreeBuilder* tree) {
    int i = 0;
    for (const auto& field: var.compound_fields()) {
        if (netCDF::type_is_user_defined(field.type)) {
            if (_usertypes.find(field.type) == _usertypes.end()) {
                throw std::runtime_error(
                        "compund type definition not found for member variable \"" + field.name + "\" of struct " +
                        var.name());
            }

            auto type_definition = _usertypes.at(field.type);
            auto new_parent_node = uda_capnp_get_child(tree, node, i);
            uda_capnp_set_node_name(new_parent_node, field.name.c_str());
            uda_capnp_add_children(new_parent_node, type_definition.compound_fields().size());
            walk_compound_type(type_definition, bytes_data_array + field.offset, new_parent_node, tree);
        } else {
            auto child_node = uda_capnp_get_child(tree, node, i);
            unpack_compound_field(var.compound_fields()[i], child_node, bytes_data_array);
        }
        i++;
    }
}

void add_nc_atomic_variable(netCDF::Variable& variable, NodeBuilder* node) {
    auto sizes = variable.sizes();
    size_t data_len = 1;
    for (const auto& elem: sizes) data_len *= elem;

    switch (variable.type()) {
        case NC_SHORT:
            uda_capnp_add_md_array_i16(node, variable.get<int16_t>().data(), sizes.data(), sizes.size());
            break;
        case NC_INT:
            uda_capnp_add_md_array_i32(node, variable.get<int32_t>().data(), sizes.data(), sizes.size());
            break;
        case NC_INT64:
            uda_capnp_add_md_array_i64(node, (int64_t*) variable.get<long long>().data(), sizes.data(), sizes.size());
            break;
        case NC_FLOAT:
            uda_capnp_add_md_array_f32(node, variable.get<float>().data(), sizes.data(), sizes.size());
            break;
        case NC_DOUBLE:
            uda_capnp_add_md_array_f64(node, variable.get<double>().data(), sizes.data(), sizes.size());
            break;
        case NC_CHAR:
            uda_capnp_add_md_array_char(node, variable.get<char>().data(), sizes.data(), sizes.size());
            break;
        case NC_STRING: {
            // array strings?
            uda_capnp_add_array_char(node, variable.get<char>().data(), data_len);
            break;
        }
        case NC_USHORT:
            uda_capnp_add_md_array_u16(node, variable.get<uint16_t>().data(), sizes.data(), sizes.size());
            break;
        case NC_UINT:
            uda_capnp_add_md_array_u32(node, variable.get<uint32_t>().data(), sizes.data(), sizes.size());
            break;
        case NC_UINT64:
            uda_capnp_add_md_array_u64(node, (uint64_t*) variable.get<unsigned long long>().data(), sizes.data(),
                                       sizes.size());
            break;
        default:
            throw std::runtime_error("unknown type");
    }
}


void Reader::handle_atomic_variable(netCDF::Variable& variable, NodeBuilder* node, TreeBuilder* tree) {
    auto attributes = variable.attributes();
    auto dims = variable.dimensions();

    // extra nodes for dims and data
    size_t offset = 2;
    size_t n_children = attributes.size() + offset;

    // 3 children for attributes, dims, and data
    uda_capnp_add_children(node, n_children);

    auto data_node = uda_capnp_get_child(tree, node, 0);
    uda_capnp_set_node_name(data_node, CAPNP_DATA_NODE_NAME);
    add_nc_atomic_variable(variable, data_node);

    auto dims_node = uda_capnp_get_child(tree, node, 1);
    uda_capnp_set_node_name(dims_node, CAPNP_DIM_NODE_NAME);

    /*
        TODO but not pressing
        can't seem to have array of arrays (i.e. vector<std::string>) type in uda_capnp
        I think this can be fixed by passing shape to uda_capnp_add_array_Xyy instead of size as it is currently.
        later then it might be nicer to change this to be one node with a list of dim names
        instead of a list of nodes each with one name

        later:  dims.data = ["dim1", "dim2", "dim3"]

        now: dims.children
                0. name = "dim1", data(bool) = hasData?
                1. name = "dim2", data(bool) = hasData?
                2. name = "dim3", data(bool) = hasData?
    */
    auto sizes = variable.sizes();
    uda_capnp_add_children(dims_node, sizes.size());
    for (size_t i = 0; i < sizes.size(); ++i) {
        // TODO allow searching using full path instead of name
        // danger is duplicate names in different groups
        std::string name = dims[i].name();
        std::string path = dims[i].full_path();
        auto dim_node = uda_capnp_get_child(tree, dims_node, i);
        uda_capnp_set_node_name(dim_node, name.c_str());

        bool dim_has_data = _index_dimensions.find(name) == _index_dimensions.end();
        uda_capnp_add_u16(dim_node, (uint16_t) dim_has_data);

        if (dim_has_data) {
            _request_coords.insert(name);
        }
    }

    for (size_t i = offset, j = 0; i < attributes.size() + offset; ++i, ++j) {
        auto att_capnp_node = uda_capnp_get_child(tree, node, i);
        add_nc_attribute(attributes[j], att_capnp_node);
    }
}

void Reader::add_nc_coordinate_data(NodeBuilder* node, TreeBuilder* tree) {
    size_t n_children = _request_coords.size();
    uda_capnp_add_children(node, n_children);

    // TODO not pressing maybe make the following prettier
    unsigned int i = 0;
    for (const auto& dim_name: _request_coords) {
        auto maybe_var = _coordinates.find(dim_name);
        if (maybe_var != _coordinates.end()) {
            netCDF::Variable var = maybe_var->second;
            auto dim_node = uda_capnp_get_child(tree, node, i);
            // std::string name = split(dim_name, "/").back();
            uda_capnp_set_node_name(dim_node, dim_name.c_str());

            auto attributes = var.attributes();

            // extra node for data
            size_t offset = 1;
            n_children = attributes.size() + offset;

            // children for attributes and data
            uda_capnp_add_children(dim_node, n_children);

            auto data_node = uda_capnp_get_child(tree, dim_node, 0);
            uda_capnp_set_node_name(data_node, CAPNP_DATA_NODE_NAME);
            add_nc_atomic_variable(var, data_node);

            for (size_t k = offset, j = 0; k < attributes.size() + offset; ++k, ++j) {
                auto att_capnp_node = uda_capnp_get_child(tree, dim_node, k);
                add_nc_attribute(attributes[j], att_capnp_node);
            }
            ++i;
        } else {
            throw CoordinateNotFoundError(dim_name);
        }
    }
}


std::pair<bool, Buffer> Reader::handle_request(const std::string& request) {
    TreeBuilder* capnp_tree = uda_capnp_new_tree();
    NodeBuilder* capnp_root = uda_capnp_get_root(capnp_tree);

    // if (request != "/" and request != "") // -- do for everything for consistency for now
    uda_capnp_add_children(capnp_root, 3);

    auto capnp_meta_node = uda_capnp_get_child(capnp_tree, capnp_root, 0);
    uda_capnp_set_node_name(capnp_meta_node, CAPNP_TOP_LEVEL_ATTRIBUTES_NODE_NAME);

    // auto nc_root = get_subtree(_file, "/");
    auto nc_attributes = _file.attributes();
    if (!nc_attributes.empty()) {
        uda_capnp_add_children(capnp_meta_node, nc_attributes.size());
        for (size_t i = 0; i < nc_attributes.size(); i++) {
            auto att_node = uda_capnp_get_child(capnp_tree, capnp_meta_node, i);
            uda_capnp_set_node_name(att_node, nc_attributes[i].name().c_str());
            add_nc_attribute(nc_attributes[i], att_node);
        }
    }

    auto capnp_data_node = uda_capnp_get_child(capnp_tree, capnp_root, 1);
    uda_capnp_set_node_name(capnp_data_node, CAPNP_REQUEST_NODE_NAME);

    auto capnp_dim_node = uda_capnp_get_child(capnp_tree, capnp_root, 2);
    uda_capnp_set_node_name(capnp_dim_node, CAPNP_DIM_NODE_NAME);

    _request_coords.clear();

    RequestType request_type = check_request_path(request);

    // TODO add handling for attributes and invalid paths

    switch (request_type) {
        case RequestType::VARIABLE: {
            // TODO use cache but first fix cache
            auto variable = get_variable(_file, request);
            if (variable.user_type()) handle_user_type_variable(variable, capnp_data_node, capnp_tree);
            else handle_atomic_variable(variable, capnp_data_node, capnp_tree);
            break;
        }
        case RequestType::GROUP: {
            auto nc_sub_tree = get_subtree(request);
            walk(nc_sub_tree, capnp_data_node, capnp_tree);
            break;
        }
        case RequestType::GROUP_ATTRIBUTE: {
            auto pos = request.rfind('.');
            auto parent_path = request.substr(0, pos);
            auto grp_itr = _groups.find(parent_path);

            if (grp_itr == _groups.end()) {
                std::string err = "Unknown error locating containing group: " + parent_path;
                throw std::runtime_error(err.c_str());
            }

            std::string attribute_name = request.substr(pos + 1);
            auto maybe_attribute = grp_itr->second.attribute(attribute_name);
            if (!maybe_attribute) {
                std::string err = "No attribute named " + attribute_name + " found in group " + parent_path;
                throw std::runtime_error(err.c_str());
            }
            auto attribute = maybe_attribute.require();
            uda_capnp_add_children(capnp_data_node, 1);
            auto capnp_request_att_node = uda_capnp_get_child(capnp_tree, capnp_data_node, 0);
            add_nc_attribute(attribute, capnp_request_att_node);
            break;
        }
        case RequestType::VARIABLE_ATTRIBUTE: {
            auto pos = request.rfind('.');
            auto parent_path = request.substr(0, pos);
            auto var_itr = _variables.find(parent_path);

            if (var_itr == _variables.end()) {
                std::string err = "Unknown error locating attribute-containing variable: " + parent_path;
                throw std::runtime_error(err.c_str());
            }

            std::string attribute_name = request.substr(pos + 1);
            auto maybe_attribute = var_itr->second.attribute(attribute_name);
            if (!maybe_attribute) {
                std::string err = "No attribute named " + attribute_name + " found in variable " + parent_path;
                throw std::runtime_error(err.c_str());
            }
            auto attribute = maybe_attribute.require();
            uda_capnp_add_children(capnp_data_node, 1);
            auto capnp_request_att_node = uda_capnp_get_child(capnp_tree, capnp_data_node, 0);
            add_nc_attribute(attribute, capnp_request_att_node);
            break;
        }
        case RequestType::INVALID_PATH: {
            throw std::runtime_error(std::string("Requested path ") + request + " does not exist");
        }
        default:
            throw std::runtime_error("unknown error");
    }

    add_nc_coordinate_data(capnp_dim_node, capnp_tree);

    _request_coords.clear();

    auto buffer = uda_capnp_serialise(capnp_tree);
    bool structured = request_is_structured_data(request);
    return std::make_pair(structured, buffer);
}
}
