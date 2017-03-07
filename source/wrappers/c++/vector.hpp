//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_VECTOR_H
#define IDAM_WRAPPERS_CPP_VECTOR_H

#include <string>
#include <vector>
#include <boost/any.hpp>

namespace uda {

class Vector
{
public:
    template <typename T>
    Vector(T * array, size_t size) : vec_(array, array+size), type_(&typeid(T)), isnull_(false)
    {}

    const std::type_info& type() const { return *type_; }

    bool isNull() const { return isnull_; }

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
    Vector() : vec_(), type_(&typeid(void)), isnull_(true) {}
private:
    std::vector<boost::any> vec_;
    const std::type_info * type_;
    bool isnull_;

    template <typename T>
    struct AnyCastTransform
    {
        T operator()(const boost::any& src) const
        {
            std::string name = src.type().name();
            return boost::any_cast<T>(src);
        }
    };
};

}

#endif //IDAM_WRAPPERS_CPP_VECTOR_H
