#include "read_netcdf.hpp"

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "nc_check.hpp"

using namespace uda::plugins::netcdf;

namespace {

std::string type_name_of(int ncid, nc_type type) {
    char name[NC_MAX_NAME + 1] = {};
    size_t size = 0;
    check(nc_inq_type(ncid, type, name, &size), "failed to inspect type");
    return name;
}

// Resolve enum types to their underlying atomic type. Other user defined types are returned unchanged.
nc_type resolve_type(int ncid, nc_type type) {
    if (type > NC_MAX_ATOMIC_TYPE) {
        nc_type base = NC_NAT;
        int type_class = 0;
        check(nc_inq_user_type(ncid, type, nullptr, nullptr, &base, nullptr, &type_class),
                "failed to inspect user defined type");
        if (type_class == NC_ENUM) {
            return base;
        }
    }
    return type;
}

template <typename T>
Value read_numeric(size_t count, const std::function<int(void*)>& reader, const std::string& context) {
    std::vector<T> values(count);
    if (count > 0) {
        check(reader(values.data()), context);
    }
    return values;
}

/**
 * Read `count` elements of the given (resolved) type using `reader`, which must fill the supplied buffer.
 */
Value read_values(nc_type type, size_t count, const std::function<int(void*)>& reader, const std::string& context) {
    switch (type) {
        case NC_BYTE:
            return read_numeric<int8_t>(count, reader, context);
        case NC_UBYTE:
            return read_numeric<uint8_t>(count, reader, context);
        case NC_SHORT:
            return read_numeric<int16_t>(count, reader, context);
        case NC_USHORT:
            return read_numeric<uint16_t>(count, reader, context);
        case NC_INT:
            return read_numeric<int32_t>(count, reader, context);
        case NC_UINT:
            return read_numeric<uint32_t>(count, reader, context);
        case NC_INT64:
            return read_numeric<int64_t>(count, reader, context);
        case NC_UINT64:
            return read_numeric<uint64_t>(count, reader, context);
        case NC_FLOAT:
            return read_numeric<float>(count, reader, context);
        case NC_DOUBLE:
            return read_numeric<double>(count, reader, context);
        case NC_CHAR: {
            std::string chars(count, '\0');
            if (count > 0) {
                check(reader(chars.data()), context);
            }
            return chars;
        }
        case NC_STRING: {
            std::vector<std::string> strings;
            if (count > 0) {
                std::vector<char*> pointers(count, nullptr);
                check(reader(pointers.data()), context);
                strings.reserve(count);
                for (char* ptr : pointers) {
                    strings.emplace_back(ptr != nullptr ? ptr : "");
                }
                nc_free_string(count, pointers.data());
            }
            return strings;
        }
        default:
            // compound, vlen, opaque etc. are not supported yet
            return std::monostate{};
    }
}

Attribute read_attribute(int ncid, int varid, const std::string& name) {
    Attribute attribute;
    attribute.name = name;

    const std::string context = "failed to read attribute '" + name + "'";
    check(nc_inq_att(ncid, varid, name.c_str(), &attribute.type, &attribute.length), context);
    attribute.type_name = type_name_of(ncid, attribute.type);

    nc_type type = resolve_type(ncid, attribute.type);
    attribute.value = read_values(
            type, attribute.length,
            [&](void* buffer) {
                return type == NC_STRING ? nc_get_att_string(ncid, varid, name.c_str(), static_cast<char**>(buffer))
                                         : nc_get_att(ncid, varid, name.c_str(), buffer);
            },
            context);
    return attribute;
}

std::vector<Attribute> read_attributes(int ncid, int varid) {
    int num_attributes = 0;
    check(nc_inq_varnatts(ncid, varid, &num_attributes), "failed to count attributes");

    std::vector<Attribute> attributes;
    attributes.reserve(num_attributes);
    for (int i = 0; i < num_attributes; ++i) {
        char name[NC_MAX_NAME + 1] = {};
        check(nc_inq_attname(ncid, varid, i, name), "failed to read attribute name");
        attributes.push_back(read_attribute(ncid, varid, name));
    }
    return attributes;
}

Variable read_variable(int ncid, int varid) {
    Variable variable;

    char name[NC_MAX_NAME + 1] = {};
    int num_dims = 0;
    int dim_ids[NC_MAX_VAR_DIMS] = {};
    check(nc_inq_var(ncid, varid, name, &variable.type, &num_dims, dim_ids, nullptr), "failed to inspect variable");
    variable.name = name;
    variable.type_name = type_name_of(ncid, variable.type);

    const std::string context = "failed to read variable '" + variable.name + "'";

    size_t count = 1;
    for (int i = 0; i < num_dims; ++i) {
        char dim_name[NC_MAX_NAME + 1] = {};
        size_t dim_len = 0;
        check(nc_inq_dim(ncid, dim_ids[i], dim_name, &dim_len), context);
        variable.dimension_names.emplace_back(dim_name);
        variable.shape.push_back(dim_len);
        count *= dim_len;
    }

    variable.attributes = read_attributes(ncid, varid);

    nc_type type = resolve_type(ncid, variable.type);
    variable.data = read_values(
            type, count, [&](void* buffer) { return nc_get_var(ncid, varid, buffer); }, context);
    return variable;
}

Group read_group(int ncid) {
    Group group;

    char name[NC_MAX_NAME + 1] = {};
    check(nc_inq_grpname(ncid, name), "failed to read group name");
    group.name = name;

    // dimensions defined in this group
    int num_dims = 0;
    check(nc_inq_dimids(ncid, &num_dims, nullptr, 0), "failed to count dimensions");
    std::vector<int> dim_ids(num_dims);
    check(nc_inq_dimids(ncid, &num_dims, dim_ids.data(), 0), "failed to list dimensions");

    int num_unlimited = 0;
    check(nc_inq_unlimdims(ncid, &num_unlimited, nullptr), "failed to count unlimited dimensions");
    std::vector<int> unlimited_ids(num_unlimited);
    check(nc_inq_unlimdims(ncid, &num_unlimited, unlimited_ids.data()), "failed to list unlimited dimensions");

    for (int dim_id : dim_ids) {
        char dim_name[NC_MAX_NAME + 1] = {};
        Dimension dimension;
        check(nc_inq_dim(ncid, dim_id, dim_name, &dimension.size), "failed to inspect dimension");
        dimension.name = dim_name;
        dimension.unlimited = std::find(unlimited_ids.begin(), unlimited_ids.end(), dim_id) != unlimited_ids.end();
        group.dimensions.push_back(std::move(dimension));
    }

    group.attributes = read_attributes(ncid, NC_GLOBAL);

    // variables
    int num_vars = 0;
    check(nc_inq_varids(ncid, &num_vars, nullptr), "failed to count variables");
    std::vector<int> var_ids(num_vars);
    check(nc_inq_varids(ncid, &num_vars, var_ids.data()), "failed to list variables");
    for (int var_id : var_ids) {
        group.variables.push_back(read_variable(ncid, var_id));
    }

    // sub-groups
    int num_groups = 0;
    check(nc_inq_grps(ncid, &num_groups, nullptr), "failed to count groups");
    std::vector<int> group_ids(num_groups);
    check(nc_inq_grps(ncid, &num_groups, group_ids.data()), "failed to list groups");
    for (int group_id : group_ids) {
        group.groups.push_back(std::make_unique<Group>(read_group(group_id)));
    }

    return group;
}

std::vector<std::string> split_path(std::string_view path) {
    std::vector<std::string> components;
    size_t pos = 0;
    while (pos < path.size()) {
        size_t next = path.find('/', pos);
        if (next == std::string_view::npos) {
            next = path.size();
        }
        if (next > pos) {
            components.emplace_back(path.substr(pos, next - pos));
        }
        pos = next + 1;
    }
    return components;
}

} // anon namespace

/**
 * Read the contents of a netCDF4 file into a structured tree, independent of UDA.
 *
 * @param file_path path to the netCDF file
 * @param data_path "/" to read the whole file, otherwise a path to a group, a variable, or an attribute,
 *                  e.g. "/group/subgroup", "/group/variable", "/group/variable/attribute" or "/group/attribute"
 * @return a Group (recursively containing all dimensions, attributes, variables and sub-groups),
 *         a Variable (with its attributes and data), or an Attribute
 * @throws std::runtime_error if the file cannot be opened/read or the path does not exist
 */
Node uda::plugins::netcdf::read_netcdf_tree(const FileHandle& file, const std::string& data_path) {
    int ncid = file.ncid();
    int varid = NC_GLOBAL;
    bool at_variable = false;

    const std::vector<std::string> components = split_path(data_path);

    for (size_t i = 0; i < components.size(); ++i) {
        const std::string& component = components[i];
        const bool last = (i + 1 == components.size());

        if (at_variable) {
            // only an attribute can follow a variable, and it must be the final component
            if (!last || nc_inq_attid(ncid, varid, component.c_str(), nullptr) != NC_NOERR) {
                throw std::runtime_error("path '" + data_path + "' not found in file '" + file.path() + "'");
            }
            return read_attribute(ncid, varid, component);
        }

        int id = 0;
        if (nc_inq_ncid(ncid, component.c_str(), &id) == NC_NOERR) {
            ncid = id;
        } else if (nc_inq_varid(ncid, component.c_str(), &id) == NC_NOERR) {
            varid = id;
            at_variable = true;
        } else if (last && nc_inq_attid(ncid, NC_GLOBAL, component.c_str(), nullptr) == NC_NOERR) {
            return read_attribute(ncid, NC_GLOBAL, component);
        } else {
            throw std::runtime_error("path '" + data_path + "' not found in file '" + file.path() + "'");
        }
    }

    if (at_variable) {
        return read_variable(ncid, varid);
    }
    return read_group(ncid);
}
