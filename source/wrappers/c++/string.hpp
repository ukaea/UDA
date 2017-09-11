//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_STRING_H
#define IDAM_WRAPPERS_CPP_STRING_H

#include <cstring>
#include <typeinfo>
#include <string>

#include "data.hpp"

namespace uda {

class String : public Data
{
public:
    String(const char * data)
            : Data(false), str_(data)
    { }

    String(const std::string& str)
            : Data(false), str_(str)
    { }

    size_t size() const
    { return str_.size(); }

    const std::type_info& type() const
    { return typeid(char*); }

    const std::string& str() const
    { return str_; }

private:
    std::string str_;
};

}

#endif //IDAM_WRAPPERS_CPP_STRING_H
