#ifdef _WIN32
// this needs to be included until we can remove the need to include <rpc> in udaClient.h, i.e after removal of XDR
// globals.
#  include <winsock.h>
#endif

#include "array.hpp"
#include "result.hpp"

uda::Array uda::Array::Null = uda::Array();

const std::vector<uda::Dim>& uda::Array::dims() const
{
    if (!dims_loaded_) {
        dim_type rank = result_->rank();

        dims_.reserve(rank);
        for (dim_type i = 0; i < rank; ++i) {
            dims_.emplace_back(result_->dim(i, uda::Result::DataType::DATA));
        }
    }

    return dims_;
}

std::size_t uda::Array::size() const
{
    if (result_ == nullptr) {
        size_t sz = 1;
        for (const auto& dim : dims()) {
            sz *= dim.size();
        }
        return sz;
    } else {
        return result_->size();
    }
}

const std::vector<std::size_t> uda::Array::shape() const
{
    if (result_ == nullptr) {
        std::vector<std::size_t> shape;
        for (const auto& dim : dims()) {
            shape.push_back(dim.size());
        }
        return shape;
    } else {
        return result_->shape();
    }
}
