#ifndef UDA_WRAPPERS_CPP_VECTOR_H
#define UDA_WRAPPERS_CPP_VECTOR_H

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <boost/any.hpp>

#include <uda/export.h>
#include "data.hpp"

#if defined(_WIN32)
#  if !defined(__GNUC__)
#    pragma warning(push)
#    pragma warning(disable: 4251)
#  endif
#endif

namespace uda {

class LIBRARY_API Vector : public Data {
public:
    template <typename T>
    Vector(T* array, size_t size)
            : Data(false)
            , vec_(array, array + size)
            , type_(&typeid(T))
            , data_size_(size * sizeof(T))
    {
        auto copy = new unsigned char[size * sizeof(T)];
        memcpy(copy, array, size * sizeof(T));
        raw_data_ = std::shared_ptr<unsigned char>(copy, std::default_delete<unsigned char[]>());
    }

    Vector(const Vector& other) = default;
    Vector& operator=(const Vector& other) = default;

    [[nodiscard]] const std::type_info& type() const override
    { return *type_; }

    template <typename T>
    std::vector<T> as() const
    {
        std::vector<T> out(vec_.size());
        std::transform(vec_.begin(), vec_.end(), out.begin(), AnyCastTransform<T>());
        return out;
    }

    template <typename T>
    T at(size_t idx) const
    { return boost::any_cast<T>(vec_[idx]); }

    [[nodiscard]] size_t size() const override
    { return vec_.size(); }

    static Vector Null;

    [[nodiscard]] const unsigned char* byte_data() const override
    {
        return raw_data_.get();
    }

    [[nodiscard]] size_t byte_length() const override
    {
        return data_size_;
    }

protected:
    Vector() noexcept
            : Data(true)
            , vec_()
            , type_(&typeid(void))
            , raw_data_{}
            , data_size_(0)
    {}

private:
    std::vector<boost::any> vec_;
    const std::type_info* type_;
    std::shared_ptr<unsigned char> raw_data_;
    size_t data_size_;

    template <typename T>
    struct AnyCastTransform {
        T operator()(const boost::any& src) const
        {
            return boost::any_cast<T>(src);
        }
    };
};

}

#if defined(_WIN32) && !defined(__GNUC__)
#  pragma warning(pop)
#endif

#endif // UDA_WRAPPERS_CPP_VECTOR_H
