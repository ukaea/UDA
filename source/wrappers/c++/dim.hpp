#ifndef IDAM_WRAPPERS_CPP_DIM_H
#define IDAM_WRAPPERS_CPP_DIM_H

#include <string>
#include <boost/multi_array/base.hpp>

#include "vector.hpp"

namespace uda {

typedef unsigned int dim_type;

class Dim {
public:
    template <typename T>
    Dim(dim_type num, T* array, size_t size, const std::string& label, const std::string& units)
            : vec_(array, array + size), type_(&typeid(T)), num_(num), label_(label), units_(units), isnull_(false)
    {}

    Dim(dim_type num, size_t size, const std::string& label, const std::string& units)
            : vec_(), type_(&typeid(int)), num_(num), label_(label), units_(units), isnull_(false)
    {
        vec_.resize(size);
        for (int i = 0; i < static_cast<int>(size); ++i) {
            vec_[i] = i;
        }
    }

    Dim() : type_(nullptr), num_(0), label_(), units_(), isnull_(true)
    {}

    ~Dim() = default;

    template <typename T>
#ifndef SWIG
    std::vector<T> as() const
#else
    std::vector<T> _as() const
#endif
    {
        std::vector<T> out(vec_.size());
        std::transform(vec_.begin(), vec_.end(), out.begin(), AnyCastTransform<T>());
        return out;
    }

    const std::type_info& type() const
    {
        return *type_;
    }

    template <typename T>
    T at(size_t idx) const
    { return boost::any_cast<T>(vec_[idx]); }

    static Dim Null;

    dim_type num() const
    { return num_; }

    const std::string& label() const
    { return label_; }

    const std::string& units() const
    { return units_; }

    size_t size() const
    { return vec_.size(); }

    bool isNull() const
    { return isnull_; }

private:

    std::vector<boost::any> vec_;
    const std::type_info* type_;
    dim_type num_;
    std::string label_;
    std::string units_;
    bool isnull_;

    template <typename T>
    struct AnyCastTransform {
        T operator()(const boost::any& src) const
        {
            std::string name = src.type().name();
            return boost::any_cast<T>(src);
        }
    };
};

}

#endif //IDAM_WRAPPERS_CPP_DIM_H
