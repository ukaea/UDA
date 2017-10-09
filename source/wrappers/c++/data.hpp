#ifndef UDA_WRAPPERS_CPP_DATA_H
#define UDA_WRAPPERS_CPP_DATA_H

#include <typeinfo>
#include <cstddef>

namespace uda {

class Data
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

protected:
    bool isnull_;
};

}

#endif // UDA_WRAPPERS_CPP_DATA_H
