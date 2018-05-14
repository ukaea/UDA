#ifndef UDA_TEST_HELPERS_H
#define UDA_TEST_HELPERS_H

#include <string>
#include <vector>
#include <sstream>

#include "catch.hpp"

namespace Catch {
namespace Detail {

class ApproxVector {
public:
    explicit ApproxVector(std::vector<double> vector)
            : vector_(std::move(vector))
    {}

    explicit ApproxVector(std::vector<float> vector)
            : vector_()
    {
        std::copy(vector.begin(), vector.end(), std::back_inserter(vector_));
    }

    ApproxVector(ApproxVector const& other)
            : vector_(other.vector_)
    {}

    template <typename T, typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    friend bool operator==(const std::vector<T>& lhs, ApproxVector const& rhs)
    {
        if (lhs.size() != rhs.vector_.size()) return false;
        size_t i = 0;
        for (const auto& val : lhs) {
            if (val != Approx(rhs.vector_[i++])) return false;
        }
        return true;
    }

    template <typename T, typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    friend bool operator==(ApproxVector const& lhs, const T& rhs)
    {
        return operator==(rhs, lhs);
    }

    template <typename T, typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    friend bool operator!=(T lhs, ApproxVector const& rhs)
    {
        return !operator==(lhs, rhs);
    }

    template <typename T, typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    friend bool operator!=(ApproxVector const& lhs, T rhs)
    {
        return !operator==(rhs, lhs);
    }

    std::string toString() const
    {
        std::ostringstream oss;
        oss << "ApproxVector( " << Catch::toString(vector_) << " )";
        return oss.str();
    }

private:
    std::vector<double> vector_;
};

}

template<>
inline std::string toString<Detail::ApproxVector>( Detail::ApproxVector const& value ) {
    return value.toString();
}

}

using Catch::Detail::ApproxVector;

namespace uda {
namespace test {
std::string format(const char* fmt, ...);
}
}

#endif //UDA_TEST_HELPERS_H
