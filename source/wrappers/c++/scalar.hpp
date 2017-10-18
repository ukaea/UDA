#ifndef UDA_WRAPPERS_CPP_SCALAR_H
#define UDA_WRAPPERS_CPP_SCALAR_H

#include <typeinfo>
#include <boost/any.hpp>

#include "data.hpp"

namespace uda {

class Scalar : public Data
{
public:
    template<typename T>
    explicit Scalar(T value)
            : Data(false)
            , value_(value)
            , type_(&typeid(T))
            , data_size_(sizeof(T))
    {
        auto copy = new T;
        *copy = value;
        raw_data_ = std::shared_ptr<unsigned char>(reinterpret_cast<unsigned char*>(copy));
    }

    Scalar(const Scalar& other) = default;
    Scalar& operator=(const Scalar& other) = default;

    size_t size() const override
    { return 0; }

    const std::type_info& type() const override
    { return *type_; }

    template<typename T>
    T as() const
    { return boost::any_cast<T>(value_); }

    static Scalar Null;

    const unsigned char* byte_data() const override
    {
        return raw_data_.get();
    }

    size_t byte_length() const override
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

#endif // UDA_WRAPPERS_CPP_SCALAR_H
