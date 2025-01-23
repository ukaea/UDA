#ifndef UDA_WRAPPERS_CPP_SCALAR_H
#define UDA_WRAPPERS_CPP_SCALAR_H

#include <memory>
#include <typeinfo>
#include <boost/any.hpp>

#include <uda/export.h>
#include "data.hpp"

#if defined(_WIN32)
#  if !defined(__GNUC__)
#    pragma warning(push)
#    pragma warning(disable: 4251)
#  endif
#endif

namespace uda {

class LIBRARY_API Scalar : public Data
{
public:
    template<typename T>
    explicit Scalar(T value)
            : Data(false)
            , value_(value)
            , type_(&typeid(T))
            , data_size_(sizeof(T))
    {
        auto copy = new unsigned char[sizeof(T)];
        memcpy(copy, &value, sizeof(T));
        raw_data_ = std::shared_ptr<unsigned char>(copy, std::default_delete<unsigned char[]>());
    }

    Scalar(const Scalar& other) = default;
    Scalar& operator=(const Scalar& other) = default;

    [[nodiscard]] size_t size() const override
    { return 0; }

    [[nodiscard]] const std::type_info& type() const override
    { return *type_; }

    template<typename T>
    T as() const
    { return boost::any_cast<T>(value_); }

    static Scalar Null;

    [[nodiscard]] const unsigned char* byte_data() const override
    {
        return raw_data_.get();
    }

    [[nodiscard]] size_t byte_length() const override
    {
        return data_size_;
    }

private:
    Scalar()
            : Data(true)
            , value_()
            , type_(&typeid(void))
            , raw_data_{}
            , data_size_(0)
    { }

    boost::any value_;
    const std::type_info* type_;
    std::shared_ptr<unsigned char> raw_data_;
    size_t data_size_;
};

}

#if defined(_WIN32) && !defined(__GNUC__)
#  pragma warning(pop)
#endif

#endif // UDA_WRAPPERS_CPP_SCALAR_H
