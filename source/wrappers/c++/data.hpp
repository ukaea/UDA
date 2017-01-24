//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_DATA_H
#define IDAM_WRAPPERS_CPP_DATA_H

#include <typeinfo>
#include <cstddef>

namespace Idam {

class Data
{
public:
    Data(bool isnull) : isnull_(isnull)
    { }

    virtual ~Data()
    { }

    bool isNull() const
    { return isnull_; }

    virtual std::size_t size() const = 0;

    virtual const std::type_info& type() const = 0;

protected:
    bool isnull_;
};

}

#endif //IDAM_WRAPPERS_CPP_DATA_H
