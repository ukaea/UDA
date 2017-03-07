#include "array.hpp"

uda::Array uda::Array::Null = uda::Array();

std::size_t uda::Array::size() const
{
    std::size_t sz = 1u;
    for (std::size_t i = 0u; i < dims_.size(); ++i) {
        sz *= dims_[i].size();
    }
    return sz;
}

