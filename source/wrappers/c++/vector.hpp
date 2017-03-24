#ifndef IDAM_WRAPPERS_CPP_VECTOR_H
#define IDAM_WRAPPERS_CPP_VECTOR_H

#include <string>
#include <vector>
#include <boost/any.hpp>

#include "data.hpp"

namespace uda {

class Vector : public Data
{
public:
    template <typename T>
    Vector(T * array, size_t size) : Data(false), vec_(array, array+size), type_(&typeid(T))
    {}

    const std::type_info& type() const { return *type_; }

    template <typename T>
    std::vector<T> as() const
    {
        std::vector<T> out(vec_.size());
        std::transform(vec_.begin(), vec_.end(), out.begin(), AnyCastTransform<T>());
        return out;
    }

    template <typename T>
    T at(size_t idx) const { return boost::any_cast<T>(vec_[idx]); }

    size_t size() const { return vec_.size(); }

    static Vector Null;
protected:
    Vector() : Data(true), vec_(), type_(&typeid(void)) {}
private:
    std::vector<boost::any> vec_;
    const std::type_info * type_;

    template <typename T>
    struct AnyCastTransform
    {
        T operator()(const boost::any& src) const
        {
            return boost::any_cast<T>(src);
        }
    };
};

}

#endif //IDAM_WRAPPERS_CPP_VECTOR_H
