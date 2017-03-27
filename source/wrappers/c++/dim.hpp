//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_DIM_H
#define IDAM_WRAPPERS_CPP_DIM_H

#include <string>
#include <boost/multi_array/base.hpp>

#include "vector.hpp"

namespace uda {

typedef unsigned int dim_type;

class Dim : public Vector
{
public:
    template <typename T>
    Dim(dim_type num, T * array, size_t size, const std::string& label, const std::string& units)
            : Vector(array, size)
            , num_(num)
            , label_(label)
            , units_(units)
    {}
    Dim() : Vector(), num_(0), label_(), units_() {}
    ~Dim() {}

    static Dim Null;

    dim_type num() const { return num_; }
    const std::string& label() const { return label_; }
    const std::string& units() const { return units_; }
private:
    dim_type num_;
    std::string label_;
    std::string units_;
};

}

#endif //IDAM_WRAPPERS_CPP_DIM_H
