#ifndef UDA_WRAPPERS_CPP_ARRAY_H
#define UDA_WRAPPERS_CPP_ARRAY_H

#include <vector>
#include <boost/any.hpp>

#include "dim.hpp"
#include "data.hpp"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#  if !defined(__GNUC__)
#    pragma warning(push)
#    pragma warning(disable: 4251)
#  endif
#else
#  define LIBRARY_API
#endif

namespace uda {

class Client;
class Result;

class LIBRARY_API Array : public Data {
public:
    template <typename T>
    Array(T* data, const uda::Result* result)
            : Data(false)
            , data_(data)
            , result_(result)
            , dims_loaded_(false)
            , dims_()
            , type_(&typeid(T))
            , raw_data_(reinterpret_cast<unsigned char*>(data))
            , data_size_(sizeof(T))
    {}

    template <typename T>
    Array(T* data, std::vector<uda::Dim> dims)
            : Data(false)
            , data_(data)
            , result_(nullptr)
            , dims_loaded_(true)
            , dims_(std::move(dims))
            , type_(&typeid(T))
            , raw_data_(reinterpret_cast<unsigned char*>(data))
            , data_size_(sizeof(T))
    {}

    Array(const Array& other) = default;
    Array& operator=(const Array& other) = default;

    std::size_t size() const override;

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

    const std::vector<std::size_t> shape() const;

    const std::vector<Dim>& dims() const;

    static Array Null;

    const unsigned char* byte_data() const override
    {
        return raw_data_;
    }

    std::size_t byte_length() const override
    {
        return data_size_ * size();
    }

private:
    friend class uda::Client;

    explicit Array() noexcept
            : Data(true)
            , data_()
            , result_(nullptr)
            , dims_loaded_(true)
            , type_(&typeid(void))
            , raw_data_{}
            , data_size_(0)
    {}

    boost::any data_;
    const uda::Result* result_;
    bool dims_loaded_;
    mutable std::vector<Dim> dims_;
    const std::type_info* type_;
    const unsigned char* raw_data_;
    std::size_t data_size_;
};

}

#if defined(_WIN32) && !defined(__GNUC__)
#  pragma warning(pop)
#endif

#endif // UDA_WRAPPERS_CPP_ARRAY_H
