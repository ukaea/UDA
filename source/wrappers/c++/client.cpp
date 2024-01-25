#include "client.hpp"

#include <boost/format.hpp>
#include <complex>
#include <iterator>

#include "accAPI.h"
#include "client.h"
#include "initStructs.h"
#include "udaGetAPI.h"
#include "udaPutAPI.h"
#include "udaTypes.h"

#include "data.hpp"
#include "result.hpp"
#include "signal.hpp"

void uda::Client::setProperty(Property prop, bool value)
{
    std::string name;

    switch (prop) {
        case PROP_DATADBLE:
            name = "get_datadble";
            break;
        case PROP_DIMDBLE:
            name = "get_dimdble";
            break;
        case PROP_TIMEDBLE:
            name = "get_timedble";
            break;
        case PROP_BYTES:
            name = "get_bytes";
            break;
        case PROP_BAD:
            name = "get_bad";
            break;
        case PROP_META:
            name = "get_meta";
            break;
        case PROP_ASIS:
            name = "get_asis";
            break;
        case PROP_UNCAL:
            name = "get_uncal";
            break;
        case PROP_NOTOFF:
            name = "get_notoff";
            break;
        case PROP_SYNTHETIC:
            name = "get_synthetic";
            break;
        case PROP_SCALAR:
            name = "get_scalar";
            break;
        case PROP_NODIMDATA:
            name = "get_nodimdata";
            break;
        case PROP_VERBOSE:
            name = "verbose";
            break;
        case PROP_DEBUG:
            name = "debug";
            break;
        case PROP_ALTDATA:
            name = "altdata";
            break;

        case PROP_TIMEOUT:
        case PROP_ALTRANK:
            throw InvalidUseException("Cannot set boolean value for non-boolean property");

        default:
            throw UDAException("Unknown property");
    }

    value ? udaSetProperty(name.c_str()) : reudaSetProperty(name.c_str());
}

void uda::Client::setProperty(Property prop, int value)
{
    std::string name;

    switch (prop) {
        case PROP_DATADBLE:
        case PROP_DIMDBLE:
        case PROP_TIMEDBLE:
        case PROP_BYTES:
        case PROP_BAD:
        case PROP_META:
        case PROP_ASIS:
        case PROP_UNCAL:
        case PROP_NOTOFF:
        case PROP_SYNTHETIC:
        case PROP_SCALAR:
        case PROP_NODIMDATA:
        case PROP_VERBOSE:
        case PROP_DEBUG:
        case PROP_ALTDATA:
            throw InvalidUseException("Cannot set integer value for boolean property");

        case PROP_TIMEOUT:
            name = (boost::format("timeout=%1%") % value).str();
            udaSetProperty(name.c_str(), udaClientFlags());
            break;
        case PROP_ALTRANK:
            name = (boost::format("altrank=%1%") % value).str();
            udaSetProperty(name.c_str(), udaClientFlags());
            break;

        default:
            throw UDAException("Unknown property");
    }
}

void uda::Client::close()
{
    udaFreeAll();
}

int uda::Client::property(Property prop)
{
    switch (prop) {
        case PROP_DATADBLE:
            return udaGetProperty("get_datadble");
        case PROP_DIMDBLE:
            return udaGetProperty("get_dimdble");
        case PROP_TIMEDBLE:
            return udaGetProperty("get_timedble");
        case PROP_BYTES:
            return udaGetProperty("get_bytes");
        case PROP_BAD:
            return udaGetProperty("get_bad");
        case PROP_META:
            return udaGetProperty("get_meta");
        case PROP_ASIS:
            return udaGetProperty("get_asis");
        case PROP_UNCAL:
            return udaGetProperty("get_uncal");
        case PROP_NOTOFF:
            return udaGetProperty("get_notoff");
        case PROP_SYNTHETIC:
            return udaGetProperty("get_synthetic");
        case PROP_SCALAR:
            return udaGetProperty("get_scalar");
        case PROP_NODIMDATA:
            return udaGetProperty("get_nodimdata");
        case PROP_VERBOSE:
            return udaGetProperty("verbose");
        case PROP_DEBUG:
            return udaGetProperty("debug");
        case PROP_ALTDATA:
            return udaGetProperty("altdata");
        case PROP_TIMEOUT:
            return udaGetProperty("timeout");
        case PROP_ALTRANK:
            return udaGetProperty("altrank");

        default:
            throw UDAException("Unknown property");
    }
}

void uda::Client::setServerHostName(const std::string& hostName)
{
    udaPutServerHost(hostName.c_str());
}

void uda::Client::setServerPort(int portNumber)
{
    udaPutServerPort(portNumber);
}

std::string uda::Client::serverHostName()
{
    return udaGetServerHost();
}

int uda::Client::serverPort()
{
    return udaGetServerPort();
}

[[noreturn]] void generate_exception()
{
    UDA_ERROR_STACK* errorstack = getUdaServerErrorStack();
    std::vector<std::string> backtrace;
    int code = errorstack->nerrors > 0 ? errorstack->idamerror[0].code : 0;
    std::string msg = errorstack->nerrors > 0 ? errorstack->idamerror[0].msg : "";

    backtrace.reserve(errorstack->nerrors);
    for (unsigned int i = 0; i < errorstack->nerrors; ++i) {
        backtrace.push_back(std::string("[") + errorstack->idamerror[i].location +
                            "]: " + errorstack->idamerror[i].msg);
    }

    if ((code > 0 && code < 25) || (code > 60 && code < 66)) {
        throw uda::ProtocolException(msg, backtrace);
    } else {
        throw uda::ServerException(msg, backtrace);
    }
}

const uda::Result& uda::Client::get(const std::string& signalName, const std::string& dataSource)
{
    auto result = new Result(udaGetAPI(signalName.c_str(), dataSource.c_str()));

    if (result->errorCode() != OK) {
        delete result;
        generate_exception();
    }

    results_.push_back(result);
    return *result;
}

uda::ResultList uda::Client::get_batch(const std::vector<std::string>& signals, const std::string& source)
{
    std::vector<std::pair<std::string, std::string>> requests;
    for (const auto& signal : signals) {
        requests.emplace_back(std::make_pair(signal, source));
    }
    return get_batch(requests);
}

uda::ResultList uda::Client::get_batch(const std::vector<std::pair<std::string, std::string>>& requests)
{
    std::vector<const char*> c_signals;
    std::vector<const char*> c_sources;
    for (const auto& request : requests) {
        c_signals.push_back(request.first.c_str());
        c_sources.push_back(request.second.c_str());
    }

    std::vector<int> handles(requests.size());
    std::fill(handles.begin(), handles.end(), -1);
    int rc = udaGetBatchAPI(c_signals.data(), c_sources.data(), (int)requests.size(), handles.data());

    if (rc != 0) {
        generate_exception();
    }

    std::unordered_map<int, size_t> indices;
    for (auto handle : handles) {
        auto result = new Result(handle);
        if (result->errorCode() != OK) {
            delete result;
            generate_exception();
        }
        indices[handle] = results_.size();
        results_.push_back(result);
    }

    return ResultList(indices, *this);
}

uda::ResultList::ResultList(std::unordered_map<int, size_t> indices, Client& client)
    : indices_{std::move(indices)}, client_(client)
{
}

const uda::Result& uda::ResultList::at(int handle) const
{
    size_t index = indices_.at(handle);
    return client_.at(index);
}

std::vector<int> uda::ResultList::handles() const
{
    std::vector<int> keys;
    for (const auto& el : indices_) {
        keys.push_back(el.first);
    }
    return keys;
}

const uda::Result& uda::Client::at(size_t index)
{
    return *results_.at(index);
}

static int typeIDToUDAType(const std::type_info& type)
{
    if (type == typeid(char)) {
        return UDA_TYPE_CHAR;
    }
    if (type == typeid(short)) {
        return UDA_TYPE_SHORT;
    }
    if (type == typeid(int)) {
        return UDA_TYPE_INT;
    }
    if (type == typeid(unsigned int)) {
        return UDA_TYPE_UNSIGNED_INT;
    }
    if (type == typeid(long)) {
        return UDA_TYPE_LONG;
    }
    if (type == typeid(float)) {
        return UDA_TYPE_FLOAT;
    }
    if (type == typeid(double)) {
        return UDA_TYPE_DOUBLE;
    }
    if (type == typeid(unsigned char)) {
        return UDA_TYPE_UNSIGNED_CHAR;
    }
    if (type == typeid(unsigned short)) {
        return UDA_TYPE_UNSIGNED_SHORT;
    }
    if (type == typeid(unsigned long)) {
        return UDA_TYPE_UNSIGNED_LONG;
    }
    if (type == typeid(long long)) {
        return UDA_TYPE_LONG64;
    }
    if (type == typeid(unsigned long long)) {
        return UDA_TYPE_UNSIGNED_LONG64;
    }
    if (type == typeid(std::complex<float>)) {
        return UDA_TYPE_COMPLEX;
    }
    if (type == typeid(std::complex<double>)) {
        return UDA_TYPE_DCOMPLEX;
    }
    if (type == typeid(char*)) {
        return UDA_TYPE_STRING;
    }

    return UDA_TYPE_UNKNOWN;
}

// typedef struct PutDataBlock {
//     int data_type;
//     unsigned int rank;
//     unsigned int count;
//     int* shape;
//     const char* data;
//     int opaque_type;                // Identifies the Data Structure Type;
//     int opaque_count;               // Number of Instances of the Data Structure;
//     void* opaque_block;             // Opaque pointer to Hierarchical Data Structures
//     unsigned int blockNameLength;   // Size of the Name character string
//     char* blockName;                // Name of the Data Block
// } PUTDATA_BLOCK;

void uda::Client::put(const uda::Signal& signal)
{
    std::string filename = (boost::format("%s%06d.nc") % signal.alias() % signal.shot()).str();

    std::string signal_class;
    switch (signal.signalClass()) {
        case uda::ANALYSED:
            signal_class = "Analysed";
            break;
        case uda::RAW:
            signal_class = "Raw";
            break;
        case uda::MODELLED:
            signal_class = "Modelled";
            break;
    }

    std::string request =
        (boost::format("putdata::open(/create,"
                       " filename='%s',"
                       " conventions='Fusion-1.0',"
                       " class='%s',"
                       " title='%s',"
                       " shot=%d,"
                       " pass=%d,"
                       " comment='%s',"
                       " code=%s,"
                       " version=1)") %
         filename % signal_class % signal.title() % signal.shot() % signal.pass() % signal.comment() % signal.code())
            .str();

    udaPutAPI(request.c_str(), nullptr);

    PUTDATA_BLOCK pdblock{};
    initIdamPutDataBlock(&pdblock);

    const uda::Array& array = signal.array();

    pdblock.data_type = typeIDToUDAType(array.type());
    pdblock.rank = (unsigned int)array.dims().size();
    pdblock.count = (unsigned int)array.size();

    std::vector<int> shape(pdblock.rank);
    pdblock.shape = shape.data();

    pdblock.data = (char*)array.byte_data();

    udaPutAPI("", &pdblock);
}

template <typename T> void put_scalar(const std::string& instruction, T data)
{
    PUTDATA_BLOCK putdata_block{};
    initIdamPutDataBlock(&putdata_block);

    putdata_block.data_type = typeIDToUDAType(typeid(T));
    putdata_block.rank = 0;
    putdata_block.count = 1;
    putdata_block.shape = nullptr;

    auto dp = new T;
    *dp = data;

    putdata_block.data = reinterpret_cast<char*>(dp);

    udaPutAPI(instruction.c_str(), &putdata_block);

    delete dp;
}

void uda::Client::put(const std::string& instruction, char data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, int8_t data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, int16_t data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, int32_t data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, int64_t data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, uint8_t data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, uint16_t data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, uint32_t data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, uint64_t data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, float data)
{
    put_scalar(instruction, data);
}
void uda::Client::put(const std::string& instruction, double data)
{
    put_scalar(instruction, data);
}

template <typename T> void put_vector(const std::string& instruction, const std::vector<T>& data)
{
    PUTDATA_BLOCK putdata_block{};
    initIdamPutDataBlock(&putdata_block);

    putdata_block.data_type = typeIDToUDAType(typeid(T));
    putdata_block.rank = 1;
    putdata_block.count = data.size();

    auto shape = new int[1];
    shape[0] = static_cast<int>(data.size());

    putdata_block.shape = shape;

    putdata_block.data = reinterpret_cast<char*>(const_cast<T*>(data.data()));

    udaPutAPI(instruction.c_str(), &putdata_block);

    delete[] shape;
}

void uda::Client::put(const std::string& instruction, const std::vector<char>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<int8_t>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<int16_t>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<int32_t>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<int64_t>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<uint8_t>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<uint16_t>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<uint32_t>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<uint64_t>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<float>& data)
{
    put_vector(instruction, data);
}
void uda::Client::put(const std::string& instruction, const std::vector<double>& data)
{
    put_vector(instruction, data);
}

uda::Client::~Client()
{
    for (auto& data : results_) {
        delete (data);
    }
    //    udaFreeAll(nullptr, nullptr);
}

void uda::Client::put(const std::string& instruction, const uda::Array& data)
{
    // Use to convert size_t to int
    struct Downcast {
        int operator()(size_t s) const { return static_cast<int>(s); }
    };

    PUTDATA_BLOCK putdata_block{};
    initIdamPutDataBlock(&putdata_block);

    putdata_block.data_type = typeIDToUDAType(data.type());
    putdata_block.rank = static_cast<unsigned int>(data.dims().size());
    putdata_block.count = static_cast<unsigned int>(data.size());

    std::vector<size_t> array_shape = data.shape();

    std::vector<int> shape;
    // C++ error: conversion from 'size_t' to 'const int', possible loss of data
    // std::copy(array_shape.begin(), array_shape.end(), std::back_inserter(shape));
    std::transform(array_shape.begin(), array_shape.end(), std::back_inserter(shape), Downcast());

    putdata_block.shape = shape.data();

    putdata_block.data = reinterpret_cast<const char*>(data.byte_data());

    udaPutAPI(instruction.c_str(), &putdata_block);
}
