//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_SCALAR_H
#define IDAM_WRAPPERS_CPP_SCALAR_H

#include <typeinfo>
#include <boost/any.hpp>

#include "data.hpp"

namespace uda {

class Scalar : public Data
{
public:
    template<typename T>
    Scalar(T value) : Data(false), value_(value), type_(&typeid(T))
    { }

    size_t size() const
    { return 0; }

    const std::type_info& type() const
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

#endif //IDAM_WRAPPERS_CPP_SCALAR_H
