#ifndef UDA_WRAPPERS_CPP_ARRAY_H
#define UDA_WRAPPERS_CPP_ARRAY_H

#include <vector>
#include <boost/any.hpp>

#include "dim.hpp"
#include "data.hpp"

namespace uda {

class Client;

class Array : public Data
{
public:
    template<typename T>
    Array(T* data, std::vector<Dim> dims)
            : Data(false), data_(data), dims_(dims), type_(&typeid(T)), raw_data_(reinterpret_cast<char*>(data))
    { }

    size_t size() const;

    const std::type_info& type() const
    {
        return *type_;
    }

    template<typename T>
    std::vector<T> as() const
    {
        T * data = boost::any_cast<T *>(data_);
        return std::vector<T>(data, data + size());
    }

    const std::vector<size_t> shape() const;

    const std::vector<Dim>& dims() const
    {
        return dims_;
    }

    static Array Null;
private:
    friend class uda::Client;

    Array() : Data(true), dims_(), type_(&typeid(void))
    { }

    boost::any data_;
    std::vector<Dim> dims_;
    const std::type_info* type_;
    const char* raw_data_;

    const char* data() const { return raw_data_; };
};

}

#endif // UDA_WRAPPERS_CPP_ARRAY_H
