#ifndef UDA_WRAPPERS_CPP_ARRAY_H
#define UDA_WRAPPERS_CPP_ARRAY_H

#include <vector>
#include <boost/any.hpp>

#include "dim.hpp"
#include "data.hpp"

namespace uda {

class Client;
class Result;

class Array : public Data {
public:
    template <typename T>
    Array(T* data, const uda::Result* result)
            : Data(false)
            , data_(data)
            , result_(result)
            , dims_loaded_(false)
            , dims_()
            , type_(&typeid(T))
            , raw_data_(reinterpret_cast<char*>(data))
    {}

    template <typename T>
    Array(T* data, std::vector<uda::Dim> dims)
            : Data(false)
            , data_(data)
            , result_(nullptr)
            , dims_loaded_(true)
            , dims_(std::move(dims))
            , type_(&typeid(T))
            , raw_data_(reinterpret_cast<char*>(data))
    {}

    size_t size() const override;

    const std::type_info& type() const override
    {
        return *type_;
    }

    template <typename T>
    std::vector<T> as() const
    {
        T* data = boost::any_cast<T*>(data_);
        return std::vector<T>(data, data + size());
    }

    const std::vector<size_t> shape() const;

    const std::vector<Dim>& dims() const;

    static Array Null;

private:
    friend class uda::Client;

    explicit Array() noexcept
            : Data(true)
            , data_()
            , result_(nullptr)
            , dims_loaded_(true)
            , type_(&typeid(void))
            , raw_data_{}
    {}

    boost::any data_;
    const uda::Result* result_;
    bool dims_loaded_;
    mutable std::vector<Dim> dims_;
    const std::type_info* type_;
    const char* raw_data_;

    const char* data() const
    { return raw_data_; };
};

}

#endif // UDA_WRAPPERS_CPP_ARRAY_H
