/*
  Copyright (c) 2020 Sven Willner <sven.willner@yfx.de>

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef NETCDFPP_H
#define NETCDFPP_H

#include <netcdf.h>

#include <cstdint>
#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace netCDF {

constexpr bool type_is_user_defined(nc_type nc_type_id) {
    return nc_type_id > std::max({NC_BYTE, NC_CHAR, NC_SHORT, NC_INT, NC_INT64, NC_FLOAT, NC_DOUBLE, NC_UBYTE,
                                  NC_USHORT, NC_UINT, NC_UINT64, NC_VLEN, NC_COMPOUND, NC_OPAQUE, NC_ENUM, NC_STRING});
}

template<int I>
struct SystemType {
};
template<typename T>
struct Type {
    static constexpr bool is_atomic = false;
};

//template<>
//struct Type<long long> {
//    static constexpr nc_type id = NC_INT64;
//    static constexpr bool is_atomic = true;
//};
//
//template<>
//struct Type<unsigned long long> {
//    static constexpr nc_type id = NC_UINT64;
//    static constexpr bool is_atomic = true;
//};

#define NETCDFPP_IMPL_TYPE(internal, lib)       \
    template<>                                  \
    struct SystemType<lib> {                    \
        using type = internal;                  \
    };                                          \
    template<>                                  \
    struct Type<internal> {                     \
        static constexpr nc_type id = lib;      \
        static constexpr bool is_atomic = true; \
    };

NETCDFPP_IMPL_TYPE(char*, NC_STRING)
NETCDFPP_IMPL_TYPE(char, NC_CHAR)
NETCDFPP_IMPL_TYPE(double, NC_DOUBLE)
NETCDFPP_IMPL_TYPE(float, NC_FLOAT)
NETCDFPP_IMPL_TYPE(std::int16_t, NC_SHORT)
NETCDFPP_IMPL_TYPE(std::int32_t, NC_INT)
NETCDFPP_IMPL_TYPE(std::int64_t, NC_INT64)
NETCDFPP_IMPL_TYPE(std::int8_t, NC_BYTE)
NETCDFPP_IMPL_TYPE(std::uint16_t, NC_USHORT)
NETCDFPP_IMPL_TYPE(std::uint32_t, NC_UINT)
NETCDFPP_IMPL_TYPE(std::uint64_t, NC_UINT64)
NETCDFPP_IMPL_TYPE(std::uint8_t, NC_UBYTE)

template<typename T, typename Function>
inline T for_type(nc_type t, Function&& f) {
    switch (t) {
        case NC_STRING: {
            SystemType<NC_STRING>::type type{};
            return f(type);
        }
        case NC_CHAR: {
            SystemType<NC_CHAR>::type type{};
            return f(type);
        }
        case NC_DOUBLE: {
            SystemType<NC_DOUBLE>::type type{};
            return f(type);
        }
        case NC_FLOAT: {
            SystemType<NC_FLOAT>::type type{};
            return f(type);
        }
        case NC_SHORT: {
            SystemType<NC_SHORT>::type type{};
            return f(type);
        }
        case NC_INT: {
            SystemType<NC_INT>::type type{};
            return f(type);
        }
        case NC_INT64: {
            SystemType<NC_INT64>::type type{};
            return f(type);
        }
        case NC_BYTE: {
            SystemType<NC_BYTE>::type type{};
            return f(type);
        }
        case NC_USHORT: {
            SystemType<NC_USHORT>::type type{};
            return f(type);
        }
        case NC_UINT: {
            SystemType<NC_UINT>::type type{};
            return f(type);
        }
        case NC_UINT64: {
            SystemType<NC_UINT64>::type type{};
            return f(type);
        }
        case NC_UBYTE: {
            SystemType<NC_UBYTE>::type type{};
            return f(type);
        }
        default:
            throw std::runtime_error("Unsupported type");
    }
}

class Exception final : public std::runtime_error {
private:
    int ret;

public:
    explicit Exception(int r, std::string s) : std::runtime_error(std::move(s)), ret(r) {}

    int return_code() const { return ret; }
};

class Attribute;

class Dimension;

class File;

class Group;

class UserType;

class Variable;

namespace testing {
class TestUserType;
}

namespace detail {

constexpr bool is_user_type(nc_type type) { return type >= NC_FIRSTUSERTYPEID; }

inline std::vector<std::string> process_char_vector(std::vector<char*>& buf) {
    std::vector<std::string> res;
    res.reserve(buf.size());
    std::copy(std::begin(buf), std::end(buf), std::back_inserter(res));
    nc_free_string(buf.size(), &buf[0]);
    return res;
}

inline std::vector<std::string> process_char_vector(char** buf, std::size_t len) {
    std::vector<std::string> res;
    res.reserve(len);
    std::copy(buf, buf + len, std::back_inserter(res));
    nc_free_string(len, buf);
    return res;
}

inline std::vector<const char*> process_string_vector(const std::vector<std::string>& v) {
    std::vector<const char*> buf(v.size());
    std::transform(std::begin(v), std::end(v), std::begin(buf), [](const std::string& s) { return s.c_str(); });
    return buf;
}

struct Path {
    std::string name;
    int id;
    bool is_group;
    std::shared_ptr<Path> parent;

    [[nodiscard]] std::string get_full_path() const {
        std::string res = name;
        const std::shared_ptr<Path>* current = &parent;
        while (*current) {
            res.insert(0, ((*current)->parent ? (*current)->name : "") + "/");
            current = &(*current)->parent;
        }
        return res;
    }
};

template<typename T>
struct ClassName {
};

template<>
struct ClassName<Attribute> {
    static constexpr const char* name = "Attribute";
};

template<>
struct ClassName<Dimension> {
    static constexpr const char* name = "Dimension";
};

template<>
struct ClassName<File> {
    static constexpr const char* name = "File";
};

template<>
struct ClassName<Group> {
    static constexpr const char* name = "Group";
};

template<>
struct ClassName<UserType> {
    static constexpr const char* name = "UserType";
};

template<>
struct ClassName<Variable> {
    static constexpr const char* name = "Variable";
};

class Object {
protected:
    std::shared_ptr<Path> path;

    explicit Object(std::shared_ptr<Path> path_p) : path(std::move(path_p)) {}

    static inline std::string get_error_message(int ret) {
        if (NC_ISSYSERR(ret)) {
            return std::strerror(ret);
        }
        return nc_strerror(ret);
    }

    void raise_error(int ret) const { throw Exception(ret, get_error_message(ret) + ": " + path->get_full_path()); }

    inline void check(int ret) const {
        if (ret != NC_NOERR) {
            raise_error(ret);
        }
    }

public:
    [[nodiscard]] const std::string& name() const { return path->name; }

    int id() const { return path->id; }

    std::string full_path() const { return path->get_full_path(); }
};

}  // namespace detail

template<typename T>
class Maybe final {
private:
    std::shared_ptr<detail::Path> path;

    void raise_error() const {
        throw Exception(NC_ENOTFOUND,
                        std::string(detail::ClassName<typename std::remove_const<T>::type>::name) + " not found: " +
                        path->get_full_path());
    }

public:
    explicit Maybe(std::shared_ptr<detail::Path> path_p) : path(std::move(path_p)) {}

    [[nodiscard]] inline T require() const {
        if (!valid()) {
            raise_error();
        }
        return T(path);
    }

    [[nodiscard]] inline bool valid() const { return path->id >= 0; }

    inline operator bool() const { return valid(); }
};

class Attribute final : public detail::Object {
    friend class Group;

    friend class Maybe<Attribute>;

    friend class Variable;

private:
    explicit Attribute(std::shared_ptr<detail::Path> path_p) : detail::Object(std::move(path_p)) {}

    [[nodiscard]] int ncid() const {
        if (path->parent->is_group) {
            return path->parent->id;
        } else {
            return path->parent->parent->id;
        }
    }

    [[nodiscard]] int othid() const {
        if (path->parent->is_group) {
            return NC_GLOBAL;
        } else {
            return path->parent->id;
        }
    }

    template<typename T>
    int get_internal(int ncid_p, int othid_p, const char* name_p, T* v) const {
        static_assert(!Type<T>::is_atomic, "Should only be used for user types");
        return nc_get_att(ncid_p, othid_p, name_p, v);
    }

    template<typename T>
    int set_internal(int ncid_p, int othid_p, const char* name_p, std::size_t len, const T* v);

public:
    void copy_values(const Attribute& a);

    template<typename T>
    std::vector<T> get() const {
        static_assert(!std::is_same<char*, T>::value && !std::is_same<const char*, T>::value,
                      "Use get_string() for reading string attributes");
        std::size_t len;
        check(nc_inq_attlen(ncid(), othid(), path->name.c_str(), &len));
        std::vector<T> res(len);
        check(get_internal<T>(ncid(), othid(), path->name.c_str(), &res[0]));
        return res;
    }

    [[nodiscard]] std::string get_string() const {
        std::size_t len;
        check(nc_inq_attlen(ncid(), othid(), path->name.c_str(), &len));
        std::vector<char> buf(len + 1);
        buf[len] = '\0';
        check(nc_get_att_text(ncid(), othid(), path->name.c_str(), &buf[0]));
        return {&buf[0]};
    }

    [[nodiscard]] bool is_group_attribute() const { return path->parent->is_group; }

    [[nodiscard]] Maybe<Group> parent_group() const {
        if (is_group_attribute()) {
            return Maybe<Group>(path->parent);
        }
        return Maybe<Group>(std::make_shared<detail::Path>(detail::Path{"..", -1, true, path}));
    }

    [[nodiscard]] Maybe<Variable> parent_variable() const {
        if (!is_group_attribute()) {
            return Maybe<Variable>(path->parent);
        }
        return Maybe<Variable>(std::make_shared<detail::Path>(detail::Path{"..", -1, true, path}));
    }

    void rename(std::string name) {
        check(nc_rename_att(ncid(), othid(), path->name.c_str(), name.c_str()));
        path->name = std::move(name);
    }

    template<typename T>
    void set(const std::vector<T>& v) {
        static_assert(Type<T>::is_atomic,
                      "For user type attributes use Attribute::set(const std::vector<T>& v, const UserType& type)");
        check(set_internal<T>(ncid(), othid(), path->name.c_str(), v.size(), &v[0]));
    }

    template<typename T>
    void set(const std::vector<T>& v, const UserType& type);

    template<typename T>
    void set(const T& v, const UserType& type);

    template<typename T>
    typename std::enable_if<std::is_same<std::string, T>::value, void>::type set(T v) {
        check(set_internal(ncid(), othid(), path->name.c_str(), v.length() + 1, v.c_str()));
    }

    template<typename T>
    typename std::enable_if<std::is_same<const char*, T>::value || std::is_same<char*, T>::value, void>::type set(T v) {
        check(set_internal(ncid(), othid(), path->name.c_str(), std::strlen(v) + 1, v));
    }

    template<typename T>
    typename std::enable_if<!std::is_same<std::string, T>::value && !std::is_same<const char*, T>::value &&
                            !std::is_same<char*, T>::value, void>::type set(
            T v) {
        static_assert(Type<T>::is_atomic, "For user type attributes use Attribute::set(T v, const UserType& type)");
        check(set_internal<T>(ncid(), othid(), path->name.c_str(), 1, &v));
    }

    [[nodiscard]] std::size_t size() const {
        std::size_t len;
        check(nc_inq_attlen(ncid(), othid(), path->name.c_str(), &len));
        return len;
    }

    [[nodiscard]] nc_type type() const {
        int res;
        check(nc_inq_atttype(ncid(), othid(), path->name.c_str(), &res));
        return res;
    }

    [[nodiscard]] std::string type_name() const {
        char name[NC_MAX_NAME + 1];
        check(nc_inq_type(ncid(), type(), name, nullptr));
        return name;
    }

    [[nodiscard]] Attribute require_type(const std::string& name) const {
        const auto name_l = type_name();
        if (name_l != name) {
            throw Exception(NC_EVARMETA, "Unexpected type '" + name_l + "': " + path->get_full_path());
        }
        return *this;
    }

    [[nodiscard]] Maybe<UserType> user_type() const;
};

class Dimension final : public detail::Object {
    friend class Group;

    friend class Maybe<Dimension>;

    friend class Variable;

private:
    mutable std::size_t size_m = 0;
    mutable bool size_read = false;

    explicit Dimension(std::shared_ptr<detail::Path> path_p) : detail::Object(std::move(path_p)) {}

public:
    std::size_t size() const {
        if (!size_read) {
            check(nc_inq_dimlen(path->parent->id, path->id, &size_m));
            size_read = true;
        }
        return size_m;
    }

    bool is_unlimited() const {
        int len;
        check(nc_inq_unlimdims(path->parent->id, &len, nullptr));
        std::vector<int> ids(len);
        check(nc_inq_unlimdims(path->parent->id, nullptr, &ids[0]));
        return std::find(std::begin(ids), std::end(ids), path->id) != std::end(ids);
    }

    Group parent() const;

    void rename(std::string name) {
        check(nc_rename_dim(path->parent->id, path->id, name.c_str()));
        path->name = std::move(name);
    }
};

class Group : public detail::Object {
    friend class Attribute;

    friend class Dimension;

    friend class Maybe<Group>;

    friend class UserType;

    friend class Variable;

protected:
    explicit Group(std::shared_ptr<detail::Path> path_p) : detail::Object(std::move(path_p)) {}

public:
    Attribute add_attribute(std::string name) {
        return Attribute(std::make_shared<detail::Path>(detail::Path{std::move(name), -1, false, path}));
    }

    Attribute add_attribute(const Attribute& a) {
        auto res = add_attribute(a.name());
        res.copy_values(a);
        return res;
    }

    Dimension add_dimension(std::string name) { return add_dimension(std::move(name), NC_UNLIMITED); }

    Dimension add_dimension(std::string name, std::size_t len) {
        int id;
        check(nc_def_dim(path->id, name.c_str(), len, &id));
        return Dimension(std::make_shared<detail::Path>(detail::Path{std::move(name), id, false, path}));
    }

    Dimension add_dimension(const Dimension& d) {
        return add_dimension(d.name(), d.is_unlimited() ? NC_UNLIMITED : d.size());
    }

    Group add_group(std::string name) {
        int id;
        check(nc_def_grp(path->id, name.c_str(), &id));
        return Group(std::make_shared<detail::Path>(detail::Path{std::move(name), id, true, path}));
    }

    Group add_group(const Group& g, bool variable_values = false) {
        auto res = add_group(g.name());
        res.copy_from(g, variable_values);
        return res;
    }

    UserType add_type_compound(std::string name, std::size_t bytes_size);

    template<typename T>
    UserType add_type_compound(std::string name);

    UserType add_type_enum(std::string name, nc_type basetype);

    template<typename T>
    UserType add_type_enum(std::string name);

    UserType add_type_vlen(std::string name, nc_type basetype);

    template<typename T>
    UserType add_type_vlen(std::string name);

    UserType add_type_opaque(std::string name, std::size_t bytes_size);

    UserType add_user_type(const UserType& t);

    Variable add_variable(std::string name, const UserType& type, const std::vector<int>& dims);

    Variable add_variable(std::string name, const UserType& type, const std::vector<Dimension>& dims);

    Variable add_variable(std::string name, const UserType& type, const std::vector<std::string>& dims);

    Variable add_variable(std::string name, nc_type type, const std::vector<int>& dims);

    Variable add_variable(std::string name, nc_type type, const std::vector<Dimension>& dims);

    Variable add_variable(std::string name, nc_type type, const std::vector<std::string>& dims);

    template<typename T>
    Variable add_variable(std::string name, const std::vector<int>& dims);

    template<typename T>
    Variable add_variable(std::string name, const std::vector<Dimension>& dims);

    template<typename T>
    Variable add_variable(std::string name, const std::vector<std::string>& dims);

    Variable add_variable(const Variable& v, bool with_values = false);

    [[nodiscard]] Maybe<Attribute> attribute(std::string name) const {
        auto res = std::make_shared<detail::Path>(detail::Path{std::move(name), -1, false, path});
        const auto ret = nc_inq_attid(path->id, NC_GLOBAL, res->name.c_str(), &res->id);
        if (ret != NC_ENOTATT) {
            check(ret);
        }
        return Maybe<Attribute>(std::move(res));
    }

    [[nodiscard]] std::vector<Attribute> attributes() const {
        int count;
        check(nc_inq_natts(path->id, &count));
        std::vector<Attribute> res;
        res.reserve(count);
        char name[NC_MAX_NAME + 1];
        for (int id = 0; id < count; ++id) {
            check(nc_inq_attname(path->id, NC_GLOBAL, id, name));
            res.emplace_back(
                    Attribute{std::make_shared<detail::Path>(detail::Path{std::string(name), id, false, path})});
        }
        return res;
    }

    void copy_attributes(const Group& g) {
        for (const auto& it: g.attributes()) {
            add_attribute(it);
        }
    }

    void copy_dimensions(const Group& g) {
        for (const auto& it: g.dimensions()) {
            add_dimension(it);
        }
    }

    void copy_from(const Group& g, bool variable_values = false) {
        copy_attributes(g);
        copy_dimensions(g);
        copy_user_types(g);
        copy_variables(g, variable_values);
        copy_groups(g, variable_values);
    }

    void copy_groups(const Group& g, bool variable_values = false) {
        for (const auto& it: g.groups()) {
            add_group(it, variable_values);
        }
    }

    void copy_user_types(const Group& g);

    void copy_variables(const Group& g, bool variable_values = false);

    [[nodiscard]] Maybe<Dimension> dimension(std::string name) const {
        auto res = std::make_shared<detail::Path>(detail::Path{std::move(name), -1, false, path});
        const auto ret = nc_inq_dimid(path->id, res->name.c_str(), &res->id);
        if (ret != NC_EBADDIM) {
            check(ret);
        }
        return Maybe<Dimension>(std::move(res));
    }

    [[nodiscard]] std::vector<Dimension> dimensions() const {
        int count;
        check(nc_inq_ndims(path->id, &count));

        std::vector<int> ids(count);
        check(nc_inq_dimids(path->id, &count, &ids[0], 0));

        char name[NC_MAX_NAME + 1];
        std::vector<Dimension> res;
        res.reserve(count);
        std::transform(std::begin(ids), std::end(ids), std::back_inserter(res), [&](int id) {
            check(nc_inq_dimname(path->id, id, name));
            return Dimension(std::make_shared<detail::Path>(detail::Path{name, id, false, path}));
        });
        return res;
    }

    [[nodiscard]] Maybe<Group> group(std::string name) const {
        auto res = std::make_shared<detail::Path>(detail::Path{std::move(name), -1, true, path});
        const auto ret = nc_inq_grp_ncid(path->id, res->name.c_str(), &res->id);
        if (ret != NC_ENOGRP) {
            check(ret);
        }
        return Maybe<Group>(std::move(res));
    }

    [[nodiscard]] std::vector<Group> groups() const {
        int count;
        check(nc_inq_grps(path->id, &count, nullptr));

        std::vector<int> ids(count);
        check(nc_inq_grps(path->id, nullptr, &ids[0]));

        char name[NC_MAX_NAME + 1];
        std::vector<Group> res;
        res.reserve(count);
        std::transform(std::begin(ids), std::end(ids), std::back_inserter(res), [&](int id) {
            check(nc_inq_grpname(id, name));
            return Group(std::make_shared<detail::Path>(detail::Path{name, id, true, path}));
        });
        return res;
    }

    void rename(std::string name) {
        check(nc_rename_grp(path->id, name.c_str()));
        path->name = std::move(name);
    }

    [[nodiscard]] Maybe<Group> parent() const {
        if (path->parent) {
            return Maybe<Group>(path->parent);
        }
        return Maybe<Group>(std::make_shared<detail::Path>(detail::Path{"..", -1, true, path}));
    }

    [[nodiscard]] Maybe<UserType> user_type(std::string name) const {
        auto res = std::make_shared<detail::Path>(detail::Path{std::move(name), -1, false, path});
        const auto ret = nc_inq_typeid(path->id, res->name.c_str(), &res->id);
        if (ret != NC_EBADTYPE) {
            check(ret);
        }
        if (!detail::is_user_type(res->id)) {
            res->id = -1;
        }
        return Maybe<UserType>(std::move(res));
    }

    [[nodiscard]] std::vector<UserType> user_types() const;

    [[nodiscard]] Maybe<Variable> variable(std::string name) const {
        auto res = std::make_shared<detail::Path>(detail::Path{std::move(name), -1, false, path});
        const auto ret = nc_inq_varid(path->id, res->name.c_str(), &res->id);
        if (ret != NC_ENOTVAR) {
            check(ret);
        }
        return Maybe<Variable>(std::move(res));
    }

    [[nodiscard]] std::vector<Variable> variables() const;
};

class File final : public Group {
public:
    File() : Group(std::make_shared<detail::Path>(detail::Path{"", -1, true, nullptr})) {}

    File(std::string filename, char mode) : File() { open(std::move(filename), mode); }

    File(const char* filename, char mode) : File(std::string(filename), mode) {}

    ~File() { close(); }

    void open(std::string filename, char mode) {
        close();
        path->name = std::move(filename);
        switch (mode) {
            case 'a':
                check(nc_open(path->name.c_str(), NC_WRITE, &path->id));
                break;
            case 'r':
                check(nc_open(path->name.c_str(), NC_NOWRITE, &path->id));
                break;
            case 'w':
                check(nc_create(path->name.c_str(), NC_NETCDF4 | NC_CLOBBER, &path->id));
                break;
            default:
                throw std::runtime_error("Unknown file mode");
        }
    }

    void close() {
        if (is_open()) {
            check(nc_close(path->id));
            path->id = -1;
        }
    }

    bool is_open() const { return path->id >= 0; }

    void sync() const { check(nc_sync(path->id)); }
};

class UserType final : public detail::Object {
    friend class Group;

    friend class Maybe<UserType>;

    friend class testing::TestUserType;

public:
    struct CompoundField {
        nc_type type = NC_NAT;
        std::string name;
        std::size_t offset = 0;
        std::vector<int> dimensions;
    };

private:
    mutable bool fields_read = false;
    mutable std::size_t size_m = 0;
    mutable nc_type basetype_m = 0;
    mutable std::size_t fieldscount_m = 0;
    mutable int typeclass_m = 0;

    explicit UserType(std::shared_ptr<detail::Path> path_p) : detail::Object(std::move(path_p)) {}

    void read_fields() const {
        check(nc_inq_user_type(path->parent->id, path->id, nullptr, &size_m, &basetype_m, &fieldscount_m,
                               &typeclass_m));
        fields_read = true;
    }

public:
    Group parent() const { return Group(path->parent); }

    template<typename T>
    UserType add_compound_field(const std::string& name, std::size_t offset) {
        check(nc_insert_compound(path->parent->id, path->id, name.c_str(), offset, Type<T>::id));
        return *this;
    }

    template<typename T>
    UserType add_compound_field_array(const std::string& name, std::size_t offset, const std::vector<int>& dim_sizes) {
        check(nc_insert_array_compound(path->parent->id, path->id, name.c_str(), offset,
                                       Type<typename std::remove_all_extents<T>::type>::id, dim_sizes.size(),
                                       &dim_sizes[0]));
        return *this;
    }

    template<typename T>
    UserType add_enum_member(const std::string& name, T v) {
        check(nc_insert_enum(path->parent->id, path->id, name.c_str(), &v));
        return *this;
    }

    std::vector<CompoundField> compound_fields() const {
        std::vector<CompoundField> res;
        res.reserve(fieldscount());
        int dims_count;
        char name[NC_MAX_NAME + 1];
        for (std::size_t i = 0; i < fieldscount(); ++i) {
            check(nc_inq_compound_fieldndims(path->parent->id, path->id, i, &dims_count));
            CompoundField f;
            f.dimensions.resize(dims_count);
            check(nc_inq_compound_field(path->parent->id, path->id, i, name, &f.offset, &f.type, nullptr,
                                        &f.dimensions[0]));
            f.name = name;
            res.emplace_back(std::move(f));
        }
        return res;
    }

    template<typename T>
    std::vector<std::pair<std::string, T>> enum_members() const {
        std::vector<std::pair<std::string, T>> res;
        res.reserve(fieldscount());
        char name[NC_MAX_NAME + 1];
        T v;
        for (std::size_t i = 0; i < fieldscount(); ++i) {
            check(nc_inq_enum_member(path->parent->id, path->id, i, name, &v));
            res.emplace_back(name, v);
        }
        return res;
    }

    nc_type basetype() const {
        if (!fields_read) {
            read_fields();
        }
        return basetype_m;
    }

    std::size_t fieldscount() const {
        if (!fields_read) {
            read_fields();
        }
        return fieldscount_m;
    }

    std::size_t memberscount() const {
        if (!fields_read) {
            read_fields();
        }
        return fieldscount_m;
    }

    std::size_t bytes_size() const {
        if (!fields_read) {
            read_fields();
        }
        return size_m;
    }

    int typeclass() const {  // NC_VLEN, NC_OPAQUE, NC_ENUM, or NC_COMPOUND
        if (!fields_read) {
            read_fields();
        }
        return typeclass_m;
    }
};

template<typename T>
struct VLenElement {
    const std::size_t size;
    const T* data;

    VLenElement() : size(0), data(nullptr) {}

    ~VLenElement() { free(const_cast<T*>(data)); }
};

class Variable final : public detail::Object {
    friend class Group;

    friend class Maybe<Variable>;

private:
    explicit Variable(std::shared_ptr<detail::Path> path_p) : detail::Object(std::move(path_p)) {}

    [[nodiscard]] std::vector<int> dimension_ids() const {
        std::vector<int> ids(dimension_count());
        check(nc_inq_vardimid(path->parent->id, path->id, &ids[0]));
        return ids;
    }

    std::size_t size(const std::size_t* /* start */, const std::size_t* count) const {
        int dims_count;
        check(nc_inq_varndims(path->parent->id, path->id, &dims_count));

        std::size_t res = 1;
        for (int i = 0; i < dims_count; ++i) {
            res *= count[i];
        }
        return res;
    }

public:
    Attribute add_attribute(std::string name) {
        return Attribute(std::make_shared<detail::Path>(detail::Path{std::move(name), -1, false, path}));
    }

    Attribute add_attribute(const Attribute& a) {
        auto res = add_attribute(a.name());
        res.copy_values(a);
        return res;
    }

    [[nodiscard]] Maybe<Attribute> attribute(std::string name) const {
        auto res = std::make_shared<detail::Path>(detail::Path{std::move(name), -1, false, path});
        const auto ret = nc_inq_attid(path->parent->id, path->id, res->name.c_str(), &res->id);
        if (ret != NC_ENOTATT) {
            check(ret);
        }
        return Maybe<Attribute>(std::move(res));
    }

    [[nodiscard]] std::vector<Attribute> attributes() const {
        int count;
        check(nc_inq_varnatts(path->parent->id, path->id, &count));
        std::vector<Attribute> res;
        res.reserve(count);
        char name[NC_MAX_NAME + 1];
        for (int id = 0; id < count; ++id) {
            check(nc_inq_attname(path->parent->id, path->id, id, name));
            res.emplace_back(
                    Attribute{std::make_shared<detail::Path>(detail::Path{std::string(name), id, false, path})});
        }
        return res;
    }

    [[nodiscard]] bool check_dimensions(const std::vector<std::string>& names) const {
        const auto& dims = dimensions();
        if (dims.size() != names.size()) {
            return false;
        }
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (dims[i].name() != names[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] Variable require_dimensions(const std::vector<std::string>& names) const {
        if (!check_dimensions(names)) {
            throw Exception(NC_EVARMETA, "Unexpected dimensions: " + path->get_full_path());
        }
        return *this;
    }

    // inline Maybe<Variable> coordinate_variable(std::string name) const
    // {
    //     auto maybe_group = parent();
    //     while(maybe_group())
    //     {
    //         auto current_group = maybe_group.require();
    //         auto maybe_var = current_group.variable(name);
    //         if (maybe_var and maybe_var.require().dimension(name))
    //         {
    //             auto dims = maybe_var.require().dimensions();
    //             if (dims.size() == 1 and dims[0].name() == name)
    //             {
    //                 return maybe_var;
    //             }
    //         }
    //         maybe_group = current_group.parent();
    //     }
    //     return Maybe<Variable>(std::make_shared<detail::Path>(detail::Path{std::move(name), -1, false, path}));
    // }

    void copy_attributes(const Variable& v) {
        for (const auto& it: v.attributes()) {
            add_attribute(it);
        }
    }

    void copy_values(const Variable& v) {
        std::size_t this_type_len;
        check(nc_inq_type(path->parent->id, type(), nullptr, &this_type_len));
        std::size_t oth_type_len;
        check(nc_inq_type(v.path->parent->id, v.type(), nullptr, &oth_type_len));

        if (this_type_len != oth_type_len) {
            throw Exception(NC_EVARMETA, "Variable type sizes do not match: " + path->get_full_path() + " and " +
                                         v.path->get_full_path());
        }

        const auto this_sizes = sizes();
        const auto oth_sizes = v.sizes();
        if (this_sizes.size() != oth_sizes.size()) {
            throw Exception(NC_EVARMETA, "Variable dimension counts do not match: " + path->get_full_path() + " and " +
                                         v.path->get_full_path());
        }
        for (std::size_t i = 0; i < oth_sizes.size(); ++i) {
            if (this_sizes[i] != 0  // not unlimited dimension or already written to
                && this_sizes[i] != oth_sizes[i]) {
                throw Exception(NC_EVARMETA, "Variable sizes do not match: " + path->get_full_path() + " and " +
                                             v.path->get_full_path());
            }
        }

        std::vector<std::size_t> index(this_sizes.size(), 0);

        std::vector<char> buf(this_type_len * v.size());
        check(nc_get_vara(v.path->parent->id, v.path->id, &index[0], &oth_sizes[0], &buf[0]));
        check(nc_put_vara(path->parent->id, path->id, &index[0], &oth_sizes[0], &buf[0]));
    }

    [[nodiscard]] std::size_t dimension_count() const {
        int count;
        check(nc_inq_varndims(path->parent->id, path->id, &count));
        return count;
    }

    [[nodiscard]] std::vector<Dimension> dimensions() const {
        const auto ids = dimension_ids();
        char name[NC_MAX_NAME + 1];
        std::vector<Dimension> res;
        res.reserve(ids.size());
        std::transform(std::begin(ids), std::end(ids), std::back_inserter(res), [&](int id) {
            check(nc_inq_dimname(path->parent->id, id, name));
            return Dimension(std::make_shared<detail::Path>(detail::Path{name, id, false, path->parent}));
        });
        return res;
    }

    [[nodiscard]] std::vector<std::size_t> get_chunking() const {
        int mode;
        std::vector<std::size_t> res(dimension_count());
        check(nc_inq_var_chunking(path->parent->id, path->id, &mode, &res[0]));
        if (mode == NC_CONTIGUOUS) {
            res.clear();
        }
        return res;
    }

    void set_chunking(const std::vector<std::size_t>& chunks) {
        if (chunks.empty()) {
            check(nc_def_var_chunking(path->parent->id, path->id, NC_CONTIGUOUS, nullptr));
        } else {
            check(nc_def_var_chunking(path->parent->id, path->id, NC_CHUNKED, &chunks[0]));
        }
    }

    void set_default_chunking() { check(nc_def_var_chunking(path->parent->id, path->id, NC_CHUNKED, nullptr)); }

    [[nodiscard]] std::pair<bool, int> get_compression() const {
        int shuffle_filter;
        int deflate_filter;
        int deflate_level;
        check(nc_inq_var_deflate(path->parent->id, path->id, &shuffle_filter, &deflate_filter, &deflate_level));
        return std::make_pair(shuffle_filter, deflate_filter ? deflate_level : -1);
    }

    void set_compression(bool shuffle_filter, int deflate_level) {
        check(nc_def_var_deflate(path->parent->id, path->id, shuffle_filter, deflate_level < 0 ? 0 : 1, deflate_level));
    }

    [[nodiscard]] int get_endianness() const {
        int res;
        check(nc_inq_var_endian(path->parent->id, path->id, &res));
        return res;
    }

    // NC_ENDIAN_NATIVE, NC_ENDIAN_LITTLE, or NC_ENDIAN_BIG
    void set_endianness(int endianness) { check(nc_def_var_endian(path->parent->id, path->id, endianness)); }

    [[nodiscard]] bool get_checksum_enabled() const {
        int res;
        check(nc_inq_var_fletcher32(path->parent->id, path->id, &res));
        return res == NC_FLETCHER32;
    }

    void set_checksum_enabled(bool v) {
        check(nc_def_var_fletcher32(path->parent->id, path->id, v ? NC_FLETCHER32 : NC_NOCHECKSUM));
    }

    template<typename T>
    std::pair<bool, T> get_fill() const {
        int no_fill;
        T res;
        check(nc_inq_var_fill(path->parent->id, path->id, &no_fill, &res));
        return std::make_pair(!no_fill, res);
    }

    template<typename T>
    void set_fill(T v) {
        check(nc_def_var_fill(path->parent->id, path->id, 0, &v));
    }

    void unset_fill() { check(nc_def_var_fill(path->parent->id, path->id, 1, nullptr)); }

    [[nodiscard]] Group parent() const { return Group(path->parent); }

    void rename(std::string name) {
        check(nc_rename_var(path->parent->id, path->id, name.c_str()));
        path->name = std::move(name);
    }

    [[nodiscard]] std::vector<std::size_t> sizes() const {
        std::vector<std::size_t> res;
        const auto dims = dimension_ids();
        res.reserve(dims.size());
        std::transform(std::begin(dims), std::end(dims), std::back_inserter(res), [this](int id) {
            std::size_t tmp;
            check(nc_inq_dimlen(path->parent->id, id, &tmp));
            return tmp;
        });
        return res;
    }

    [[nodiscard]] std::size_t size() const {
        std::size_t res = 1;
        std::size_t tmp;
        for (const auto id: dimension_ids()) {
            check(nc_inq_dimlen(path->parent->id, id, &tmp));
            res *= tmp;
        }
        return res;
    }

    template<int N>
    std::size_t size(const std::array<std::size_t, N>& start, const std::array<std::size_t, N>& count) const {
        return size(&start[0], &count[0]);
    }

    [[nodiscard]] Variable require_size(std::size_t size_p) const {
        if (size_p != size()) {
            throw Exception(NC_EVARMETA, "Unexpected variable size: " + path->get_full_path());
        }
        return *this;
    }

    // std::string get_string()
    // {
    //     std::string res;
    //     res.reserve(size());

    // }

    template<typename T>
    std::vector<T> get() const {
        std::vector<T> res(size());
        read(&res[0]);
        return res;
    }

    template<typename T>
    T get(const std::size_t* index) const {
        T res;
        read(&res, index);
        return res;
    }

    template<typename T>
    std::vector<T> get(const std::size_t* start, const std::size_t* count) const {
        std::vector<T> res(size(start, count));
        read(&res[0], start, count);
        return res;
    }

    template<typename T>
    std::vector<T> get(const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride) const {
        std::vector<T> res(size(start, count));
        read(&res[0], start, count, stride);
        return res;
    }

    template<typename T, int N>
    T get(const std::array<std::size_t, N>& index) const {
        return get < T > (&index[0]);
    }

    template<typename T, int N>
    std::vector<T> get(const std::array<std::size_t, N>& start, const std::array<std::size_t, N>& count) const {
        return get < T > (&start[0], &count[0]);
    }

    template<typename T, int N>
    std::vector<T> get(const std::array<std::size_t, N>& start, const std::array<std::size_t, N>& count,
                       const std::array<std::ptrdiff_t, N>& stride) const {
        return get < T > (&start[0], &count[0], &stride[0]);
    }

    template<typename T>
    void read(T* v) const {
        static_assert(!Type<T>::is_atomic || std::is_same<void, T>::value,
                      "Use void or one of the explicitly supported types");
        check(nc_get_var(path->parent->id, path->id, v));
    }

    template<typename T>
    void read(T* v, const std::size_t* index) const {
        static_assert(!Type<T>::is_atomic || std::is_same<void, T>::value,
                      "Use void or one of the explicitly supported types");
        check(nc_get_var1(path->parent->id, path->id, index, v));
    }

    template<typename T>
    void read(T* v, const std::size_t* start, const std::size_t* count) const {
        static_assert(!Type<T>::is_atomic || std::is_same<void, T>::value,
                      "Use void or one of the explicitly supported types");
        check(nc_get_vara(path->parent->id, path->id, start, count, v));
    }

    template<typename T>
    void read(T* v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride) const {
        static_assert(!Type<T>::is_atomic || std::is_same<void, T>::value,
                      "Use void or one of the explicitly supported types");
        check(nc_get_vars(path->parent->id, path->id, start, count, stride, v));
    }

    template<typename T>
    void read(T* v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride,
              const std::ptrdiff_t* imap) const {
        static_assert(std::is_same<void, T>::value, "NetCDF supports mapped access only for atomic types");
        check(nc_get_varm(path->parent->id, path->id, start, count, stride, imap, v));
    }

    template<typename T, int N>
    void read(T* v, const std::array<std::size_t, N>& index) const {
        read(v, &index[0]);
    }

    template<typename T, int N>
    void read(T* v, const std::array<std::size_t, N>& start, const std::array<std::size_t, N>& count) const {
        read(v, &start[0], &count[0]);
    }

    template<typename T, int N>
    void read(T* v, const std::array<std::size_t, N>& start, const std::array<std::size_t, N>& count,
              const std::array<std::ptrdiff_t, N>& stride) const {
        read(v, &start[0], &count[0], &stride[0]);
    }

    template<typename T, int N>
    void read(T* v,
              const std::array<std::size_t, N>& start,
              const std::array<std::size_t, N>& count,
              const std::array<std::ptrdiff_t, N>& stride,
              const std::array<std::ptrdiff_t, N>& imap) const {
        read(v, &start[0], &count[0], &stride[0], &imap[0]);
    }

    template<typename T>
    void set(const std::vector<T>& v) {
        write(&v[0]);
    }

    template<typename T>
    typename std::enable_if<
            Type<T>::is_atomic || std::is_same<const char*, T>::value || std::is_same<char*, T>::value, void>::type set(
            T v, const std::size_t* index) {
        write(&v, index);
    }

    template<typename T>
    typename std::enable_if<
            !Type<T>::is_atomic && !std::is_same<const char*, T>::value && !std::is_same<char*, T>::value, void>::type
    set(
            const T& v, const std::size_t* index) {
        write(&v, index);
    }

    template<typename T>
    void set(const std::vector<T>& v, const std::size_t* start, const std::size_t* count) {
        write(&v[0], start, count);
    }

    template<typename T>
    void
    set(const std::vector<T>& v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride) {
        write(&v[0], start, count, stride);
    }

    template<typename T>
    void set(const std::vector<T>& v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride,
             const std::ptrdiff_t* imap) {
        write(&v[0], start, count, stride, imap);
    }

    template<typename T, int N>
    void set(const T v, const std::array<std::size_t, N>& index) {
        set(v, &index[0]);
    }

    template<typename T, int N>
    void
    set(const std::vector<T>& v, const std::array<std::size_t, N>& start, const std::array<std::size_t, N>& count) {
        set(v, &start[0], &count[0]);
    }

    template<typename T, int N>
    void set(const std::vector<T>& v,
             const std::array<std::size_t, N>& start,
             const std::array<std::size_t, N>& count,
             const std::array<std::ptrdiff_t, N>& stride) {
        set(v, &start[0], &count[0], &stride[0]);
    }

    template<typename T, int N>
    void set(const std::vector<T>& v,
             const std::array<std::size_t, N>& start,
             const std::array<std::size_t, N>& count,
             const std::array<std::ptrdiff_t, N>& stride,
             const std::array<std::ptrdiff_t, N>& imap) {
        set(v, &start[0], &count[0], &stride[0], &imap[0]);
    }

    template<typename T>
    void write(const T* v) {
        static_assert(!Type<T>::is_atomic || std::is_same<void, T>::value,
                      "Use void or one of the explicitly supported types");
        check(nc_put_var(path->parent->id, path->id, v));
    }

    template<typename T>
    void write(const T* v, const std::size_t* index) {
        static_assert(!Type<T>::is_atomic || std::is_same<void, T>::value,
                      "Use void or one of the explicitly supported types");
        check(nc_put_var1(path->parent->id, path->id, index, v));
    }

    template<typename T>
    void write(const T* v, const std::size_t* start, const std::size_t* count) {
        static_assert(!Type<T>::is_atomic || std::is_same<void, T>::value,
                      "Use void or one of the explicitly supported types");
        check(nc_put_vara(path->parent->id, path->id, start, count, v));
    }

    template<typename T>
    void write(const T* v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride) {
        static_assert(!Type<T>::is_atomic || std::is_same<void, T>::value,
                      "Use void or one of the explicitly supported types");
        check(nc_put_vars(path->parent->id, path->id, start, count, stride, v));
    }

    template<typename T>
    void write(const T* v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride,
               const std::ptrdiff_t* imap) {
        static_assert(std::is_same<void, T>::value, "NetCDF supports mapped access only for atomic types");
        check(nc_put_varm(path->parent->id, path->id, start, count, stride, imap, v));
    }

    template<typename T, int N>
    void write(const T* v, const std::array<std::size_t, N>& index) {
        write(v, &index[0]);
    }

    template<typename T, int N>
    void write(const T* v, const std::array<std::size_t, N>& start, const std::array<std::size_t, N>& count) {
        write(v, &start[0], &count[0]);
    }

    template<typename T, int N>
    void write(const T* v, const std::array<std::size_t, N>& start, const std::array<std::size_t, N>& count,
               const std::array<std::ptrdiff_t, N>& stride) {
        write(v, &start[0], &count[0], &stride[0]);
    }

    template<typename T, int N>
    void write(const T* v,
               const std::array<std::size_t, N>& start,
               const std::array<std::size_t, N>& count,
               const std::array<std::ptrdiff_t, N>& stride,
               const std::array<std::ptrdiff_t, N>& imap) {
        write(v, &start[0], &count[0], &stride[0], &imap[0]);
    }

    nc_type type() const {
        nc_type res;
        check(nc_inq_vartype(path->parent->id, path->id, &res));
        return res;
    }

    std::string type_name() const {
        char name[NC_MAX_NAME + 1];
        check(nc_inq_type(path->parent->id, type(), name, nullptr));
        return name;
    }

    [[nodiscard]] Variable require_type(const std::string& name) const {
        const auto name_l = type_name();
        if (name_l != name) {
            throw Exception(NC_EVARMETA, "Unexpected type '" + name_l + "': " + path->get_full_path());
        }
        return *this;
    }

    template<typename T>
    [[nodiscard]] Variable require_compound(std::size_t fieldscount) const {
        const auto user_type_l = user_type().require();
        if (user_type_l.typeclass() != NC_COMPOUND) {
            throw Exception(NC_EVARMETA,
                            "Type '" + user_type_l.name() + "' is not a compound: " + path->get_full_path());
        }
        if (user_type_l.bytes_size() != sizeof(T) || user_type_l.fieldscount() != fieldscount) {
            throw Exception(NC_EVARMETA,
                            "Unexpected size for type '" + user_type_l.name() + "': " + path->get_full_path());
        }
        return *this;
    }

    [[nodiscard]] Maybe<UserType> user_type() const {
        const auto id = type();
        char name[NC_MAX_NAME + 1];
        check(nc_inq_type(path->parent->id, id, name, nullptr));
        auto res = std::make_shared<detail::Path>(detail::Path{name, id, false, path->parent});
        if (!detail::is_user_type(res->id)) {
            res->id = -1;
            res->name = path->name + " not of user type";
        }
        return Maybe<UserType>(std::move(res));
    }
};

inline void Attribute::copy_values(const Attribute& a) {
    auto type_l = a.type();
    std::size_t type_len;
    char name[NC_MAX_NAME + 1];
    check(nc_inq_type(a.ncid(), type_l, name, &type_len));

    if (detail::is_user_type(type_l) && ncid() != a.ncid()) {
        const auto this_type = ([this, name]() {
            if (is_group_attribute()) {
                return parent_group().require().user_type(name).require();
            }
            return parent_variable().require().parent().user_type(name).require();
        })();
        type_l = this_type.id();
        if (this_type.bytes_size() != type_len) {
            throw Exception(NC_EATTMETA, "Attribute type sizes do not match: " + path->get_full_path() + " and " +
                                         a.path->get_full_path());
        }
    }

    const auto oth_size = a.size();
    std::vector<char> buf(type_len * oth_size);
    check(nc_get_att(a.ncid(), a.othid(), a.name().c_str(), &buf[0]));
    check(nc_put_att(ncid(), othid(), path->name.c_str(), type_l, oth_size, &buf[0]));
}

template<>
inline std::vector<std::string> Attribute::get() const {
    std::size_t len;
    check(nc_inq_attlen(ncid(), othid(), path->name.c_str(), &len));
    char** buf = static_cast<char**>(std::malloc(len * sizeof(char*)));
    check(nc_get_att_string(ncid(), othid(), path->name.c_str(), buf));
    return detail::process_char_vector(buf, len);
}

template<>
inline void Attribute::set(const std::vector<std::string>& v) {
    auto buf = detail::process_string_vector(v);
    check(nc_put_att_string(ncid(), othid(), path->name.c_str(), buf.size(), &buf[0]));
}

template<>
inline void Attribute::set(const std::vector<const char*>& v) {
    check(nc_put_att_string(ncid(), othid(), path->name.c_str(), v.size(), const_cast<const char**>(&v[0])));
}

template<>
inline void Attribute::set(const std::vector<char*>& v) {
    check(nc_put_att_string(ncid(), othid(), path->name.c_str(), v.size(), const_cast<const char**>(&v[0])));
}

template<typename T>
inline void Attribute::set(const std::vector<T>& v, const UserType& type) {
    static_assert(!Type<T>::is_atomic, "Should be of user type");
    check(nc_put_att(ncid(), othid(), path->name.c_str(), type.id(), v.size(), &v[0]));
}

template<typename T>
inline void Attribute::set(const T& v, const UserType& type) {
    static_assert(!Type<T>::is_atomic, "Should be of user type");
    check(nc_put_att(ncid(), othid(), path->name.c_str(), type.id(), 1, &v));
}

inline Maybe<UserType> Attribute::user_type() const {
    const auto id = type();
    char name[NC_MAX_NAME + 1];
    check(nc_inq_type(path->parent->id, id, name, nullptr));
    auto res = std::make_shared<detail::Path>(detail::Path{name, id, false, path->parent});
    if (!detail::is_user_type(res->id)) {
        res->id = -1;
        res->name = path->name + " not of user type";
    }
    return Maybe<UserType>(std::move(res));
}

inline Group Dimension::parent() const { return Group(path->parent); }

inline UserType Group::add_type_compound(std::string name, std::size_t bytes_size) {
    int id;
    check(nc_def_compound(path->id, bytes_size, name.c_str(), &id));
    return UserType(std::make_shared<detail::Path>(detail::Path{std::move(name), id, false, path}));
}

template<typename T>
inline UserType Group::add_type_compound(std::string name) {
    return add_type_compound(std::move(name), sizeof(T));
}

template<typename T>
inline UserType Group::add_type_enum(std::string name) {
    static_assert(std::is_enum<T>::value, "should be an enum class");
    return add_type_enum(std::move(name), Type<typename std::underlying_type<T>::type>::id);
}

inline UserType Group::add_type_enum(std::string name, nc_type basetype) {
    int id;
    check(nc_def_enum(path->id, basetype, name.c_str(), &id));
    return UserType(std::make_shared<detail::Path>(detail::Path{std::move(name), id, false, path}));
}

inline UserType Group::add_type_opaque(std::string name, std::size_t bytes_size) {
    int id;
    check(nc_def_opaque(path->id, bytes_size, name.c_str(), &id));
    return UserType(std::make_shared<detail::Path>(detail::Path{std::move(name), id, false, path}));
}

template<typename T>
inline UserType Group::add_type_vlen(std::string name) {
    return add_type_vlen(std::move(name), Type<T>::id);
}

inline UserType Group::add_type_vlen(std::string name, nc_type basetype) {
    int id;
    check(nc_def_vlen(path->id, name.c_str(), basetype, &id));
    return UserType(std::make_shared<detail::Path>(detail::Path{std::move(name), id, false, path}));
}

inline UserType Group::add_user_type(const UserType& t) {
    switch (t.typeclass()) {
        case NC_VLEN:
            return add_type_vlen(t.name(), t.basetype());
        case NC_OPAQUE:
            return add_type_opaque(t.name(), t.bytes_size());
        case NC_ENUM: {
            auto res = add_type_enum(t.name(), t.basetype());
            std::vector<char> buf(t.bytes_size());
            char name[NC_MAX_NAME + 1];
            for (std::size_t i = 0; i < t.fieldscount(); ++i) {
                check(nc_inq_enum_member(t.path->parent->id, t.id(), i, name, &buf[0]));
                check(nc_insert_enum(path->id, res.id(), name, &buf[0]));
            }
            return res;
        }
        case NC_COMPOUND: {
            auto res = add_type_compound(t.name(), t.bytes_size());
            for (const auto& field: t.compound_fields()) {
                if (field.dimensions.empty()) {
                    check(nc_insert_compound(path->id, res.id(), field.name.c_str(), field.offset, field.type));
                } else {
                    check(nc_insert_array_compound(path->id, res.id(), field.name.c_str(), field.offset, field.type,
                                                   field.dimensions.size(),
                                                   &field.dimensions[0]));
                }
            }
            return res;
        }
        default:
            throw Exception(0, "Invalid user type class: " + t.path->get_full_path());
    }
}

inline void Group::copy_user_types(const Group& g) {
    for (const auto& it: g.user_types()) {
        add_user_type(it);
    }
}

inline void Group::copy_variables(const Group& g, bool variable_values) {
    for (const auto& it: g.variables()) {
        add_variable(it, variable_values);
    }
}

inline Variable Group::add_variable(std::string name, const UserType& type, const std::vector<int>& dims) {
    return add_variable(std::move(name), type.id(), dims);
}

inline Variable Group::add_variable(std::string name, const UserType& type, const std::vector<Dimension>& dims) {
    return add_variable(std::move(name), type.id(), dims);
}

inline Variable Group::add_variable(std::string name, const UserType& type, const std::vector<std::string>& dims) {
    return add_variable(std::move(name), type.id(), dims);
}

inline Variable Group::add_variable(std::string name, nc_type type, const std::vector<int>& dims) {
    int id;
    check(nc_def_var(path->id, name.c_str(), type, dims.size(), &dims[0], &id));
    return Variable(std::make_shared<detail::Path>(detail::Path{std::move(name), id, false, path}));
}

inline Variable Group::add_variable(std::string name, nc_type type, const std::vector<Dimension>& dims) {
    std::vector<int> dimids(dims.size());
    std::transform(std::begin(dims), std::end(dims), std::begin(dimids), [](const Dimension& d) { return d.path->id; });
    return add_variable(std::move(name), type, dimids);
}

inline Variable Group::add_variable(std::string name, nc_type type, const std::vector<std::string>& dims) {
    std::vector<int> dimids(dims.size());
    std::transform(std::begin(dims), std::end(dims), std::begin(dimids),
                   [this](const std::string& s) { return dimension(s).require().path->id; });
    return add_variable(std::move(name), type, dimids);
}

template<typename T>
inline Variable Group::add_variable(std::string name, const std::vector<int>& dims) {
    return add_variable(std::move(name), Type<T>::id, dims);
}

template<typename T>
inline Variable Group::add_variable(std::string name, const std::vector<Dimension>& dims) {
    return add_variable(std::move(name), Type<T>::id, dims);
}

template<typename T>
inline Variable Group::add_variable(std::string name, const std::vector<std::string>& dims) {
    return add_variable(std::move(name), Type<T>::id, dims);
}

template<>
inline Variable Group::add_variable<std::string>(std::string name, const std::vector<int>& dims) {
    return add_variable(std::move(name), Type<char*>::id, dims);
}

template<>
inline Variable Group::add_variable<std::string>(std::string name, const std::vector<Dimension>& dims) {
    return add_variable(std::move(name), Type<char*>::id, dims);
}

template<>
inline Variable Group::add_variable<std::string>(std::string name, const std::vector<std::string>& dims) {
    return add_variable(std::move(name), Type<char*>::id, dims);
}

inline Variable Group::add_variable(const Variable& v, bool with_values) {
    const auto orig_dims = v.dimensions();
    std::vector<std::string> dims;
    dims.reserve(orig_dims.size());
    std::transform(std::begin(orig_dims), std::end(orig_dims), std::back_inserter(dims),
                   [](const Dimension& d) { return d.name(); });
    auto type = v.type();
    if (detail::is_user_type(type) && v.path->parent->id != path->id) {
        type = user_type(v.user_type().require().name()).require().id();
    }
    auto res = add_variable(v.name(), type, dims);
    res.copy_attributes(v);

    const auto comp = v.get_compression();
    res.set_compression(comp.first, comp.second);

    res.set_chunking(v.get_chunking());
    if (v.get_checksum_enabled()) {
        res.set_checksum_enabled(true);
    }  // otherwise chunking might get reset

    if (!detail::is_user_type(type)) {
        // fill values are handled weirdly by NetCDF
        // _FillValue has been copied via copy_attributes already
        // _NoFill seems to only properly work for fundamental types
        int no_fill;
        check(nc_inq_var_fill(v.path->parent->id, v.path->id, &no_fill, nullptr));
        if (no_fill) {
            check(nc_def_var_fill(res.path->parent->id, res.path->id, 1, nullptr));
        }
    }

    if (with_values) {
        res.copy_values(v);
    }
    return res;
}

inline std::vector<UserType> Group::user_types() const {
    int count;
    check(nc_inq_typeids(path->id, &count, nullptr));

    std::vector<int> ids(count);
    check(nc_inq_typeids(path->id, nullptr, &ids[0]));

    char name[NC_MAX_NAME + 1];
    std::vector<UserType> res;
    for (const auto id: ids) {
        if (detail::is_user_type(id)) {
            check(nc_inq_type(path->id, id, name, nullptr));
            res.emplace_back(UserType(std::make_shared<detail::Path>(detail::Path{name, id, false, path})));
        }
    }
    return res;
}

inline std::vector<Variable> Group::variables() const {
    int count;
    check(nc_inq_varids(path->id, &count, nullptr));

    std::vector<int> ids(count);
    check(nc_inq_varids(path->id, nullptr, &ids[0]));

    char name[NC_MAX_NAME + 1];
    std::vector<Variable> res;
    res.reserve(count);
    std::transform(std::begin(ids), std::end(ids), std::back_inserter(res), [&](int id) {
        check(nc_inq_varname(path->id, id, name));
        return Variable(std::make_shared<detail::Path>(detail::Path{name, id, false, path}));
    });
    return res;
}

template<>
inline std::vector<std::string> Variable::get() const {
    std::vector<char*> buf(size());
    check(nc_get_var_string(path->parent->id, path->id, &buf[0]));
    return detail::process_char_vector(buf);
}

template<>
inline std::string Variable::get(const std::size_t* index) const {
    char* buf;
    check(nc_get_var1_string(path->parent->id, path->id, index, &buf));
    std::string res(buf);
    nc_free_string(1, &buf);
    return res;
}

template<>
inline std::vector<std::string> Variable::get(const std::size_t* start, const std::size_t* count) const {
    std::vector<char*> buf(size(start, count));
    check(nc_get_vara_string(path->parent->id, path->id, start, count, &buf[0]));
    return detail::process_char_vector(buf);
}

template<>
inline std::vector<std::string>
Variable::get(const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride) const {
    std::vector<char*> buf(size(start, count));
    check(nc_get_vars_string(path->parent->id, path->id, start, count, stride, &buf[0]));
    return detail::process_char_vector(buf);
}

template<>
inline void Variable::set(const std::vector<std::string>& v) {
    auto buf = detail::process_string_vector(v);
    check(nc_put_var_string(path->parent->id, path->id, &buf[0]));
}

template<>
inline void Variable::set(const std::string& v, const std::size_t* index) {
    const char* t = v.c_str();
    check(nc_put_var1_string(path->parent->id, path->id, index, &t));
}

template<>
inline void Variable::set(const char* v, const std::size_t* index) {
    check(nc_put_var1_string(path->parent->id, path->id, index, &v));
}

template<>
inline void Variable::set(const std::vector<std::string>& v, const std::size_t* start, const std::size_t* count) {
    auto buf = detail::process_string_vector(v);
    check(nc_put_vara_string(path->parent->id, path->id, start, count, &buf[0]));
}

template<>
inline void Variable::set(const std::vector<std::string>& v, const std::size_t* start, const std::size_t* count,
                          const std::ptrdiff_t* stride) {
    auto buf = detail::process_string_vector(v);
    check(nc_put_vars_string(path->parent->id, path->id, start, count, stride, &buf[0]));
}

template<>
inline void Variable::set(
        const std::vector<std::string>& v, const std::size_t* start, const std::size_t* count,
        const std::ptrdiff_t* stride, const std::ptrdiff_t* imap) {
    auto buf = detail::process_string_vector(v);
    check(nc_put_varm_string(path->parent->id, path->id, start, count, stride, imap, &buf[0]));
}

#define NETCDFPP_IMPL_ATTRIBUTE_GET(type, name)                                                      \
    template<>                                                                                       \
    inline int Attribute::get_internal(int ncid_p, int othid_p, const char* name_p, type* v) const { \
        return nc_get_att##name(ncid_p, othid_p, name_p, v);                                         \
    }

#define NETCDFPP_IMPL_ATTRIBUTE_SET(type, name)                                                                       \
    template<>                                                                                                        \
    inline int Attribute::set_internal(int ncid_p, int othid_p, const char* name_p, std::size_t len, const type* v) { \
        return nc_put_att##name(ncid_p, othid_p, name_p, Type<type>::id, len, v);                                     \
    }

#define NETCDFPP_IMPL_VARIABLE_READ(type, name)                                                                                                               \
    template<>                                                                                                                                                \
    inline void Variable::read(type* v) const {                                                                                                               \
        check(nc_get_var##name(path->parent->id, path->id, v));                                                                                               \
    }                                                                                                                                                         \
    template<>                                                                                                                                                \
    inline void Variable::read(type* v, const std::size_t* index) const {                                                                                     \
        check(nc_get_var1##name(path->parent->id, path->id, index, v));                                                                                       \
    }                                                                                                                                                         \
    template<>                                                                                                                                                \
    inline void Variable::read(type* v, const std::size_t* start, const std::size_t* count) const {                                                           \
        check(nc_get_vara##name(path->parent->id, path->id, start, count, v));                                                                                \
    }                                                                                                                                                         \
    template<>                                                                                                                                                \
    inline void Variable::read(type* v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride) const {                             \
        check(nc_get_vars##name(path->parent->id, path->id, start, count, stride, v));                                                                        \
    }                                                                                                                                                         \
    template<>                                                                                                                                                \
    inline void Variable::read(type* v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride, const std::ptrdiff_t* imap) const { \
        check(nc_get_varm##name(path->parent->id, path->id, start, count, stride, imap, v));                                                                  \
    }

#define NETCDFPP_IMPL_VARIABLE_WRITE(type, name)                                                                                                               \
    template<>                                                                                                                                                 \
    inline void Variable::write(const type* v) {                                                                                                               \
        check(nc_put_var##name(path->parent->id, path->id, v));                                                                                                \
    }                                                                                                                                                          \
    template<>                                                                                                                                                 \
    inline void Variable::write(const type* v, const std::size_t* index) {                                                                                     \
        check(nc_put_var1##name(path->parent->id, path->id, index, v));                                                                                        \
    }                                                                                                                                                          \
    template<>                                                                                                                                                 \
    inline void Variable::write(const type* v, const std::size_t* start, const std::size_t* count) {                                                           \
        check(nc_put_vara##name(path->parent->id, path->id, start, count, v));                                                                                 \
    }                                                                                                                                                          \
    template<>                                                                                                                                                 \
    inline void Variable::write(const type* v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride) {                             \
        check(nc_put_vars##name(path->parent->id, path->id, start, count, stride, v));                                                                         \
    }                                                                                                                                                          \
    template<>                                                                                                                                                 \
    inline void Variable::write(const type* v, const std::size_t* start, const std::size_t* count, const std::ptrdiff_t* stride, const std::ptrdiff_t* imap) { \
        check(nc_put_varm##name(path->parent->id, path->id, start, count, stride, imap, v));                                                                   \
    }

#define NETCDFPP_IMPL_ALL(type, name)       \
    NETCDFPP_IMPL_ATTRIBUTE_GET(type, name) \
    NETCDFPP_IMPL_ATTRIBUTE_SET(type, name) \
    NETCDFPP_IMPL_VARIABLE_READ(type, name) \
    NETCDFPP_IMPL_VARIABLE_WRITE(type, name)

// the following maps each type to its corresponding nc_*var* (key is the type used as parameter type in the respective nc_*var*)

// NETCDFPP_IMPL_ATTRIBUTE_GET(char, _text) handled by Attribute::get_string
template<>
inline int Attribute::set_internal(int ncid_p, int othid_p, const char* name_p, std::size_t len, const char* v) {
    return nc_put_att_text(ncid_p, othid_p, name_p, len,
                           v);  // unfortunately this has a different signature than the other nc_put_att_*
}

NETCDFPP_IMPL_VARIABLE_READ(char, _text)

NETCDFPP_IMPL_VARIABLE_WRITE(char, _text)

NETCDFPP_IMPL_ALL(double, _double)

NETCDFPP_IMPL_ALL(float, _float)

NETCDFPP_IMPL_ALL(int16_t, _short)

NETCDFPP_IMPL_ALL(int32_t, _int)

NETCDFPP_IMPL_ALL(long long, _longlong)

NETCDFPP_IMPL_ALL(signed char, _schar)

NETCDFPP_IMPL_ALL(unsigned char, _uchar)

NETCDFPP_IMPL_ALL(uint16_t, _ushort)

NETCDFPP_IMPL_ALL(uint32_t, _uint)
// no netcdf-c functions exist for unsigned long types
NETCDFPP_IMPL_ALL(unsigned long long, _ulonglong)

}  // namespace netCDF

#endif
