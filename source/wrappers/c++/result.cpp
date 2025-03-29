//
// Created by jholloc on 08/03/16.
//

#include "result.hpp"

#include <complex>
#include <uda/client.h>

#include "array.hpp"
#include "string.hpp"

static const std::type_info* idamTypeToTypeID(int type)
{
    switch (type) {
        case UDA_TYPE_CHAR:
            return &typeid(char);
        case UDA_TYPE_SHORT:
            return &typeid(short);
        case UDA_TYPE_INT:
            return &typeid(int);
        case UDA_TYPE_UNSIGNED_INT:
            return &typeid(unsigned int);
        case UDA_TYPE_LONG:
            return &typeid(long);
        case UDA_TYPE_FLOAT:
            return &typeid(float);
        case UDA_TYPE_DOUBLE:
            return &typeid(double);
        case UDA_TYPE_UNSIGNED_CHAR:
            return &typeid(unsigned char);
        case UDA_TYPE_UNSIGNED_SHORT:
            return &typeid(unsigned short);
        case UDA_TYPE_UNSIGNED_LONG:
            return &typeid(unsigned long);
        case UDA_TYPE_LONG64:
            return &typeid(long long);
        case UDA_TYPE_UNSIGNED_LONG64:
            return &typeid(unsigned long long);
        case UDA_TYPE_COMPLEX:
            return &typeid(std::complex<float>);
        case UDA_TYPE_DCOMPLEX:
            return &typeid(std::complex<double>);
        case UDA_TYPE_STRING:
            return &typeid(char*);
        default:
            return &typeid(void);
    }
}

const std::string uda::Result::errorMessage() const
{
    const char* error = udaGetError(handle_);
    return error == nullptr ? "" : error;
}

int uda::Result::errorCode() const
{
    return udaGetErrorCode(handle_);
}

uda::Result::Result(int handle)
    : handle_(handle), label_(handle >= 0 ? udaGetDataLabel(handle) : ""),
      units_(handle >= 0 ? udaGetDataUnits(handle) : ""), desc_(handle >= 0 ? udaGetDataDesc(handle) : ""),
      type_(handle >= 0 ? idamTypeToTypeID(udaGetDataType(handle)) : &typeid(void)),
      uda_type_(handle >= 0 ? udaGetDataType(handle) : UDA_TYPE_UNKNOWN),
      rank_(handle >= 0 ? static_cast<dim_type>(udaGetRank(handle)) : 0),
      size_(handle >= 0 ? static_cast<std::size_t>(udaGetDataNum(handle)) : 0)
{
    if (handle >= 0 && (bool)udaGetProperty("get_meta")) {
        // TODO: fix reading all meta fields
        //        SignalDesc* signal_desc = getIdamSignalDesc(handle);
        //        meta_["signal_name"] = signal_desc->signal_name;
        //        meta_["signal_alias"] = signal_desc->signal_alias;
        //
        //        DataSource* source = getIdamDataSource(handle);
        //        meta_["path"] = source->path;
        //        meta_["filename"] = source->filename;
        //        meta_["format"] = source->format;
        //        meta_["exp_number"] = std::to_string(source->exp_number);
        //        meta_["pass"] = std::to_string(source->pass);
        //        meta_["pass_date"] = source->pass_date;
    }
    istree_ = (udaSetDataTree(handle) != 0);
}

uda::Result::~Result()
{
    udaFree(handle_);
}

const std::vector<size_t> uda::Result::shape() const
{
    size_t rank = static_cast<size_t>(udaGetRank(handle_));

    std::vector<size_t> shape(rank);

    for (size_t i = 0; i < rank; ++i) {
        shape[i] = static_cast<size_t>(udaGetDimNum(handle_, static_cast<int>(i)));
    }

    return shape;
}

template <typename T> static uda::Dim getDim(int handle, uda::dim_type num, uda::Result::DataType data_type)
{
    if (data_type == uda::Result::DataType::DATA) {
        std::string label = udaGetDimLabel(handle, num);
        std::string units = udaGetDimUnits(handle, num);
        auto size = static_cast<size_t>(udaGetDimNum(handle, num));
        auto data = reinterpret_cast<T*>(udaGetDimData(handle, num));
        return uda::Dim(num, data, size, label, units);
    }

    std::string label = udaGetDimLabel(handle, num);
    std::string units = udaGetDimUnits(handle, num);
    auto size = static_cast<size_t>(udaGetDimNum(handle, num));
    auto data = reinterpret_cast<T*>(udaGetDimError(handle, num, true));
    return uda::Dim(num, data, size, label + " error", units);
}

bool uda::Result::hasTimeDim() const
{
    return udaGetOrder(handle_) >= 0;
}

uda::Dim uda::Result::timeDim(DataType data_type) const
{
    auto order = udaGetOrder(handle_);
    if (order >= 0) {
        return dim(static_cast<dim_type>(order), data_type);
    }
    return Dim::Null;
}

uda::Dim uda::Result::dim(uda::dim_type num, DataType data_type) const
{
    int type = 0;
    if (data_type == DATA) {
        type = udaGetDimType(handle_, num);
    } else {
        type = udaGetDimErrorType(handle_, num);
    }

    switch (type) {
        case UDA_TYPE_CHAR:
            return getDim<char>(handle_, num, data_type);
        case UDA_TYPE_SHORT:
            return getDim<short>(handle_, num, data_type);
        case UDA_TYPE_INT:
            return getDim<int>(handle_, num, data_type);
        case UDA_TYPE_UNSIGNED_INT:
            return getDim<unsigned int>(handle_, num, data_type);
        case UDA_TYPE_LONG:
            return getDim<long>(handle_, num, data_type);
        case UDA_TYPE_FLOAT:
            return getDim<float>(handle_, num, data_type);
        case UDA_TYPE_DOUBLE:
            return getDim<double>(handle_, num, data_type);
        case UDA_TYPE_UNSIGNED_CHAR:
            return getDim<unsigned char>(handle_, num, data_type);
        case UDA_TYPE_UNSIGNED_SHORT:
            return getDim<unsigned short>(handle_, num, data_type);
        case UDA_TYPE_UNSIGNED_LONG:
            return getDim<unsigned long>(handle_, num, data_type);
        case UDA_TYPE_LONG64:
            return getDim<long long>(handle_, num, data_type);
        case UDA_TYPE_UNSIGNED_LONG64:
            return getDim<unsigned long long>(handle_, num, data_type);
        default:
            return Dim::Null;
    }

    return Dim::Null;
}

template <typename T> uda::Data* getDataAs(int handle, uda::Result::DataType data_type, const uda::Result* result)
{
    T* data = nullptr;
    if (data_type == uda::Result::DataType::DATA) {
        data = reinterpret_cast<T*>(udaGetData(handle));
    } else {
        data = reinterpret_cast<T*>(udaGetDataError(handle, true));
    }

    if (udaGetRank(handle) == 0) {
        if (udaGetDataNum(handle) > 1) {
            return new uda::Vector(data, (size_t)udaGetDataNum(handle));
        }
        return new uda::Scalar(data[0]);
    } else {
        return new uda::Array(data, result);
    }
}

uda::Data* getDataAsString(int handle)
{
    char* data = udaGetData(handle);

    return new uda::String(data);
}

uda::Data* getDataAsStringArray(int handle)
{
    char* data = udaGetData(handle);

    auto str_len = static_cast<size_t>(udaGetDimNum(handle, 0));
    size_t arr_len = udaGetDataNum(handle) / str_len;

    auto strings = new std::vector<std::string>;
    std::vector<uda::Dim> dims;

    auto rank = static_cast<uda::dim_type>(udaGetRank(handle));
    for (uda::dim_type dim_n = 1; dim_n < rank; ++dim_n) {
        auto dim_data = udaGetDimData(handle, dim_n);
        auto dim_size = static_cast<size_t>(udaGetDimNum(handle, dim_n));
        auto label = udaGetDimLabel(handle, dim_n);
        auto units = udaGetDimUnits(handle, dim_n);
        dims.emplace_back(
            uda::Dim(dim_n, dim_data, dim_size, label != nullptr ? label : "", units != nullptr ? units : ""));
    }

    for (size_t i = 0; i < arr_len; ++i) {
        char* str = &data[i * str_len];
        strings->push_back(std::string(str, strlen(str)));
    }

    return new uda::Array(strings->data(), dims);
}

uda::Data* uda::Result::data() const
{
    auto rank = static_cast<dim_type>(udaGetRank(handle_));

    int type = udaGetDataType(handle_);

    switch (type) {
        case UDA_TYPE_CHAR:
            return getDataAs<char>(handle_, DATA, this);
        case UDA_TYPE_SHORT:
            return getDataAs<short>(handle_, DATA, this);
        case UDA_TYPE_INT:
            return getDataAs<int>(handle_, DATA, this);
        case UDA_TYPE_UNSIGNED_INT:
            return getDataAs<unsigned int>(handle_, DATA, this);
        case UDA_TYPE_LONG:
            return getDataAs<long>(handle_, DATA, this);
        case UDA_TYPE_FLOAT:
            return getDataAs<float>(handle_, DATA, this);
        case UDA_TYPE_DOUBLE:
            return getDataAs<double>(handle_, DATA, this);
        case UDA_TYPE_UNSIGNED_CHAR:
            return getDataAs<unsigned char>(handle_, DATA, this);
        case UDA_TYPE_UNSIGNED_SHORT:
            return getDataAs<unsigned short>(handle_, DATA, this);
        case UDA_TYPE_UNSIGNED_LONG:
            return getDataAs<unsigned long>(handle_, DATA, this);
        case UDA_TYPE_LONG64:
            return getDataAs<long long>(handle_, DATA, this);
        case UDA_TYPE_UNSIGNED_LONG64:
            return getDataAs<unsigned long long>(handle_, DATA, this);
        case UDA_TYPE_STRING:
            if (rank == 1 || rank == 0) {
                return getDataAsString(handle_);
            } else {
                return getDataAsStringArray(handle_);
            }
        default:
            return &Array::Null;
    }
}

bool uda::Result::hasErrors() const
{
    return udaGetDataErrorType(handle_) != UDA_TYPE_UNKNOWN;
}

uda::Data* uda::Result::errors() const
{
    if (!hasErrors()) {
        return nullptr;
    }

    std::vector<Dim> dims;
    auto rank = static_cast<dim_type>(udaGetRank(handle_));
    for (dim_type i = 0; i < rank; ++i) {
        // XXX: error dimension data doesn't seem to actually be returned, so stick with standard dims for now
        dims.push_back(dim(i, DATA));
    }

    int type = udaGetDataErrorType(handle_);

    switch (type) {
        case UDA_TYPE_CHAR:
            return getDataAs<char>(handle_, ERRORS, this);
        case UDA_TYPE_SHORT:
            return getDataAs<short>(handle_, ERRORS, this);
        case UDA_TYPE_INT:
            return getDataAs<int>(handle_, ERRORS, this);
        case UDA_TYPE_UNSIGNED_INT:
            return getDataAs<unsigned int>(handle_, ERRORS, this);
        case UDA_TYPE_LONG:
            return getDataAs<long>(handle_, ERRORS, this);
        case UDA_TYPE_FLOAT:
            return getDataAs<float>(handle_, ERRORS, this);
        case UDA_TYPE_DOUBLE:
            return getDataAs<double>(handle_, ERRORS, this);
        case UDA_TYPE_UNSIGNED_CHAR:
            return getDataAs<unsigned char>(handle_, ERRORS, this);
        case UDA_TYPE_UNSIGNED_SHORT:
            return getDataAs<unsigned short>(handle_, ERRORS, this);
        case UDA_TYPE_UNSIGNED_LONG:
            return getDataAs<unsigned long>(handle_, ERRORS, this);
        case UDA_TYPE_LONG64:
            return getDataAs<long long>(handle_, ERRORS, this);
        case UDA_TYPE_UNSIGNED_LONG64:
            return getDataAs<unsigned long long>(handle_, ERRORS, this);
        case UDA_TYPE_STRING:
            if (rank == 1) {
                return getDataAsString(handle_);
            } else {
                return getDataAsStringArray(handle_);
            }
        default:
            return &Array::Null;
    }
}

const char* uda::Result::raw_data() const
{
    return udaGetData(handle_);
}

uda::TreeNode uda::Result::tree() const
{
    return {handle_, udaGetDataTree(handle_)};
}
