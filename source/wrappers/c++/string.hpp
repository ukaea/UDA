#ifndef UDA_WRAPPERS_CPP_STRING_H
#define UDA_WRAPPERS_CPP_STRING_H

#include <cstring>
#include <typeinfo>
#include <string>

#include "data.hpp"

namespace uda {

class String : public Data
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

private:
    std::string str_;
};

}

#endif // UDA_WRAPPERS_CPP_STRING_H
