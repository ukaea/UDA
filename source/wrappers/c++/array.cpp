//
// Created by jholloc on 08/03/16.
//

#include "array.hpp"

Idam::Array Idam::Array::Null = Idam::Array();

std::size_t Idam::Array::size() const
{
    std::size_t sz = 1u;
    for (std::size_t i = 0u; i < dims_.size(); ++i) {
        sz *= dims_[i].size();
    }
    return sz;
}

