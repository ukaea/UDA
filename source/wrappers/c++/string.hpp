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
    String(char * data)
            : Data(false), data_(data), size_(strlen(data))
    { }

    size_t size() const
    { return size_; }

    const std::type_info& type() const
    { return typeid(char *); }

    std::string str() const
    { return std::string(data_, data_ + size_); }

private:
    char * data_;
    size_t size_;
};

}

#endif //IDAM_WRAPPERS_CPP_STRING_H
