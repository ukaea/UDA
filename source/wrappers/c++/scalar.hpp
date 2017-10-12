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
    { }

    size_t size() const override
    { return 0; }

    const std::type_info& type() const override
    { return *type_; }

    template<typename T>
    T as() const
    { return boost::any_cast<T>(value_); }

    static Scalar Null;

private:
    Scalar() : Data(true), value_(), type_(&typeid(void))
    { }

    boost::any value_;
    const std::type_info* type_;
};

}

#endif // UDA_WRAPPERS_CPP_SCALAR_H
