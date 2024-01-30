#ifndef UDA_WRAPPERS_CPP_STRING_H
#define UDA_WRAPPERS_CPP_STRING_H

#include <cstring>
#include <typeinfo>
#include <string>

#include "include/uda/export.h"
#include "data.hpp"

#if defined(_WIN32)
#  if !defined(__GNUC__)
#    pragma warning(push)
#    pragma warning(disable: 4251)
#  endif
#endif

namespace uda {

class LIBRARY_API String : public Data
{
public:
    explicit String(const char * data)
            : Data(false)
            , str_(data)
    { }

    explicit String(const std::string& str)
            : Data(false), str_(str)
    { }

    size_t size() const override
    { return str_.size(); }

    const std::type_info& type() const override
    { return typeid(char*); }

    const std::string& str() const
    { return str_; }

    const unsigned char* byte_data() const override
    {
        return reinterpret_cast<const unsigned char*>(str_.data());
    }

    size_t byte_length() const override
    {
        return str_.size();
    }

private:
    std::string str_;
};

}

#if defined(_WIN32) && !defined(__GNUC__)
#  pragma warning(pop)
#endif

#endif // UDA_WRAPPERS_CPP_STRING_H
