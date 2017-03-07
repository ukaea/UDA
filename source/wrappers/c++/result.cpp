//
// Created by jholloc on 08/03/16.
//

#include "result.hpp"

#include <sstream>
#include <complex>

#include <client/accAPI.h>
#include <client/udaClient.h>
#include <clientserver/udaTypes.h>

#include "string.hpp"
#include "array.hpp"

static const std::type_info* idamTypeToTypeID(int type)
{
    switch (type) {
        case TYPE_CHAR:
            return &typeid(char);
        case TYPE_SHORT:
            return &typeid(short);
        case TYPE_INT:
            return &typeid(int);
        case TYPE_UNSIGNED_INT:
            return &typeid(unsigned int);
        case TYPE_LONG:
            return &typeid(long);
        case TYPE_FLOAT:
            return &typeid(float);
        case TYPE_DOUBLE:
            return &typeid(double);
        case TYPE_UNSIGNED_CHAR:
            return &typeid(unsigned char);
        case TYPE_UNSIGNED_SHORT:
            return &typeid(unsigned short);
        case TYPE_UNSIGNED_LONG:
            return &typeid(unsigned long);
        case TYPE_LONG64:
            return &typeid(long long);
        case TYPE_UNSIGNED_LONG64:
            return &typeid(unsigned long long);
        case TYPE_COMPLEX:
            return &typeid(std::complex<float>);
        case TYPE_DCOMPLEX:
            return &typeid(std::complex<double>);
        case TYPE_STRING:
            return &typeid(char*);
        default:
            return &typeid(void);
    }
}

const std::string uda::Result::error() const
{
    char* error = getIdamErrorMsg(handle_);
    return error == NULL ? "" : error;
}

int uda::Result::errorCode() const
{
    return getIdamErrorCode(handle_);
}

namespace {
std::string to_string(int num)
{
    std::ostringstream ss;
    ss << num;
    return ss.str();
}
}

uda::Result::Result(int handle)
        : handle_(handle), label_(handle >= 0 ? getIdamDataLabel(handle) : ""),
          units_(handle >= 0 ? getIdamDataUnits(handle) : ""),
          desc_(handle >= 0 ? getIdamDataDesc(handle) : ""),
          type_(handle >= 0 ? idamTypeToTypeID(getIdamDataType(handle)) : &typeid(void)),
          data_(handle >= 0 ? getIdamData(handle) : NULL),
          rank_(handle >= 0 ? static_cast<dim_type>(getIdamRank(handle)) : 0),
          size_(handle >= 0 ? static_cast<std::size_t>(getIdamDataNum(handle)) : 0)
{
    if (handle >= 0 && getIdamProperties(handle)->get_meta) {
        DATA_SOURCE* source = getIdamDataSource(handle);
        meta_["path"] = source->path;
        meta_["filename"] = source->filename;
        meta_["format"] = source->format;
        meta_["exp_number"] = to_string(source->exp_number);
        meta_["pass"] = to_string(source->pass);
        meta_["pass_date"] = source->pass_date;
    }
    istree_ = (setIdamDataTree(handle) != 0);
}

uda::Result::~Result()
{
    idamFree(handle_);
}

template<typename T>
static uda::Dim getDim(int handle, uda::dim_type num)
{
    std::string label = getIdamDimLabel(handle, num);
    std::string units = getIdamDimUnits(handle, num);
    int size = getIdamDimNum(handle, num);
    T* data = reinterpret_cast<T*>(getIdamDimData(handle, num));
    return uda::Dim(num, data, size, label, units);
}

uda::Dim uda::Result::dim(uda::dim_type num) const
{
    int type = getIdamDimType(handle_, num);

    switch (type) {
        case TYPE_CHAR:
            return getDim<char>(handle_, num);
        case TYPE_SHORT:
            return getDim<short>(handle_, num);
        case TYPE_INT:
            return getDim<int>(handle_, num);
        case TYPE_UNSIGNED_INT:
            return getDim<unsigned int>(handle_, num);
        case TYPE_LONG:
            return getDim<long>(handle_, num);
        case TYPE_FLOAT:
            return getDim<float>(handle_, num);
        case TYPE_DOUBLE:
            return getDim<double>(handle_, num);
        case TYPE_UNSIGNED_CHAR:
            return getDim<unsigned char>(handle_, num);
        case TYPE_UNSIGNED_SHORT:
            return getDim<unsigned short>(handle_, num);
        case TYPE_UNSIGNED_LONG:
            return getDim<unsigned long>(handle_, num);
        case TYPE_LONG64:
            return getDim<long long>(handle_, num);
        case TYPE_UNSIGNED_LONG64:
            return getDim<unsigned long long>(handle_, num);
        default:
            return Dim::Null;
    }

    return Dim::Null;
}

template<typename T>
uda::Data* getDataAs(int handle, std::vector<uda::Dim>& dims)
{
    T* data = reinterpret_cast<T*>(getIdamData(handle));

    if (getIdamRank(handle) == 0) {
        return new uda::Scalar(data[0]);
    } else {
        return new uda::Array(data, dims);
    }
}

uda::String* getDataAsString(int handle)
{
    char* data = getIdamData(handle);

    return new uda::String(data);
}

uda::Data* uda::Result::data() const
{
    std::vector<Dim> dims;
    dim_type rank = static_cast<dim_type>(getIdamRank(handle_));
    for (dim_type i = 0; i < rank; ++i) {
        dims.push_back(dim(i));
    }

    int type = getIdamDataType(handle_);

    switch (type) {
        case TYPE_CHAR:
            return getDataAs<char>(handle_, dims);
        case TYPE_SHORT:
            return getDataAs<short>(handle_, dims);
        case TYPE_INT:
            return getDataAs<int>(handle_, dims);
        case TYPE_UNSIGNED_INT:
            return getDataAs<unsigned int>(handle_, dims);
        case TYPE_LONG:
            return getDataAs<long>(handle_, dims);
        case TYPE_FLOAT:
            return getDataAs<float>(handle_, dims);
        case TYPE_DOUBLE:
            return getDataAs<double>(handle_, dims);
        case TYPE_UNSIGNED_CHAR:
            return getDataAs<unsigned char>(handle_, dims);
        case TYPE_UNSIGNED_SHORT:
            return getDataAs<unsigned short>(handle_, dims);
        case TYPE_UNSIGNED_LONG:
            return getDataAs<unsigned long>(handle_, dims);
        case TYPE_LONG64:
            return getDataAs<long long>(handle_, dims);
        case TYPE_UNSIGNED_LONG64:
            return getDataAs<unsigned long long>(handle_, dims);
        case TYPE_STRING:
            return getDataAsString(handle_);
        default:
            return &Array::Null;
    }
}

uda::TreeNode uda::Result::tree() const
{
    return TreeNode(getIdamDataTree(handle_));
}


