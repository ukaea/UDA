#include <uda.h>

#include "array.hpp"
#include "result.hpp"

uda::Array uda::Array::Null = uda::Array();

const std::vector<uda::Dim>& uda::Array::dims() const
{
    if (!dims_loaded_) {
        dims_.resize(result_->rank());

        for (dim_type i = 0; i < result_->rank(); ++i) {
            dims_.emplace_back(result_->dim(i, uda::Result::DataType::DATA));
        }
    }

    return dims_;
}

std::size_t uda::Array::size() const
{
    return result_->size();
}

const std::vector<size_t> uda::Array::shape() const
{
    return result_->shape();
}
