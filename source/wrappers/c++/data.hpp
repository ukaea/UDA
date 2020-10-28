#ifndef UDA_WRAPPERS_CPP_DATA_H
#define UDA_WRAPPERS_CPP_DATA_H

#include <typeinfo>
#include <cstddef>
#include <clientserver/export.h>

namespace uda {

class LIBRARY_API Data
{
public:
    explicit Data(bool isnull)
            : isnull_(isnull)
    { }

    virtual ~Data() = default;

    bool isNull() const
    {
        return isnull_;
    }

    virtual std::size_t size() const = 0;

    virtual const std::type_info& type() const = 0;

    virtual const unsigned char* byte_data() const = 0;
    virtual size_t byte_length() const = 0;

protected:
    bool isnull_;
};

}

#endif // UDA_WRAPPERS_CPP_DATA_H
