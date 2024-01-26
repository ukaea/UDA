#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "udaTypes.h"
#include "clientserver/initStructs.h"
#include <client2/udaGetAPI.h>
#include <client2/accAPI.h>
#include "struct.h"
#include <c++/scalar.hpp>
#include <c++/vector.hpp>
#include <c++/array.hpp>
#include <c++/result.hpp>
#include "accessors.h"

uda::Scalar uda::Scalar::Null = uda::Scalar();
uda::Vector uda::Vector::Null = uda::Vector();
uda::Array uda::Array::Null = uda::Array();
uda::Dim uda::Dim::Null = uda::Dim();

template<typename T>
static uda::Dim getDim(int handle, uda::dim_type num, uda::Result::DataType data_type)
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
    auto data = reinterpret_cast<T*>(udaGetDimError(handle, num));
    return uda::Dim(num, data, size, label + " error", units);
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

std::size_t uda::Array::size() const
{
    if (result_ == nullptr) {
        size_t sz = 1;
        for (const auto& dim : dims()) {
            sz *= dim.size();
        }
        return sz;
    } else {
        return result_->size();
    }
}

const std::vector<uda::Dim>& uda::Array::dims() const
{
    if (!dims_loaded_) {
        dim_type rank = result_->rank();

        dims_.reserve(rank);
        for (dim_type i = 0; i < rank; ++i) {
            dims_.emplace_back(result_->dim(i, uda::Result::DataType::DATA));
        }
    }

    return dims_;
}

template <typename T>
uda::Scalar getScalar(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* name)
{
    T* val = reinterpret_cast<T*>(getNodeStructureComponentData(logmalloclist, node, (char*)name));
    return uda::Scalar(*val);
}

template <>
uda::Scalar getScalar<char*>(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* name)
{
    char* val = reinterpret_cast<char*>(getNodeStructureComponentData(logmalloclist, node, (char*)name));
    return uda::Scalar(val);
}

template <>
uda::Scalar getScalar<char**>(LOGMALLOCLIST* logmalloclist, NTREE* node, const char* name)
{
    char** val = reinterpret_cast<char**>(getNodeStructureComponentData(logmalloclist, node, (char*)name));
    return uda::Scalar(val[0]);
}

uda::Scalar atomicScalar(const std::string& target, int handle, NTREE* ntree)
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    NTREE* node = findNTreeStructureComponent(logmalloclist, ntree, (char*)target.c_str()); // Locate the named variable target
    //NTREE * node = findNTreeStructureComponent(ntree, target.c_str()); // Locate the named variable target
    if (node == nullptr) {
        return uda::Scalar::Null;
    }

    int acount = getNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return uda::Scalar::Null; // No atomic data
    }

    char** anames = getNodeAtomicNames(logmalloclist, node);
    char** atypes = getNodeAtomicTypes(logmalloclist, node);
    int* arank = getNodeAtomicRank(logmalloclist, node);
    int** ashape = getNodeAtomicShape(logmalloclist, node);

    if (anames == nullptr || atypes == nullptr || arank == nullptr || ashape == nullptr) {
        return uda::Scalar::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]
            && std::string("STRING") == atypes[i]
            && (arank[i] == 0 || arank[i] == 1)) {
            return getScalar<char*>(logmalloclist, node, anames[i]);
        } else if (target == anames[i] && std::string("STRING *") == atypes[i] && arank[i] == 0) {
            return getScalar<char**>(logmalloclist, node, anames[i]);
        } else if (target == anames[i]
                   && (arank[i] == 0
                       || (arank[i] == 1 && ashape[i][0] == 1))) {
            if (std::string("short") == atypes[i]) return getScalar<short>(logmalloclist, node, anames[i]);
            if (std::string("double") == atypes[i]) return getScalar<double>(logmalloclist, node, anames[i]);
            if (std::string("float") == atypes[i]) return getScalar<float>(logmalloclist, node, anames[i]);
            if (std::string("int") == atypes[i]) return getScalar<int>(logmalloclist, node, anames[i]);
            if (std::string("char") == atypes[i]) return getScalar<char>(logmalloclist, node, anames[i]);
            if (std::string("unsigned int") == atypes[i]) return getScalar<unsigned int>(logmalloclist, node, anames[i]);
            if (std::string("unsigned short") == atypes[i]) return getScalar<unsigned short>(logmalloclist, node, anames[i]);
            if (std::string("unsigned char") == atypes[i]) return getScalar<unsigned char>(logmalloclist, node, anames[i]);
        }
    }

    return uda::Scalar::Null;
}

template <typename T>
static uda::Vector getVector(LOGMALLOCLIST* logmalloclist, NTREE* node, const std::string& target, int count)
{
    T* data = reinterpret_cast<T*>(getNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));

    return uda::Vector(data, (size_t)count);
}

uda::Vector getStringVector(LOGMALLOCLIST* logmalloclist, NTREE* node, const std::string& target, int* shape)
{
    int count = shape[1];

    auto data = static_cast<char**>(malloc(count * sizeof(char*)));
    if (data == nullptr) {
        return uda::Vector::Null;
    }

    auto val = reinterpret_cast<char*>(getNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));

    for (int j = 0; j < count; j++) {
        data[j] = &val[j * shape[0]];
    }

    return uda::Vector(data, (size_t)count);
}

uda::Vector getStringVector(LOGMALLOCLIST* logmalloclist, NTREE* node, const std::string& target)
{
    auto data = static_cast<char**>(malloc(sizeof(char*)));
    if (data == nullptr) {
        return uda::Vector::Null;
    }

    auto val = reinterpret_cast<char*>(getNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));
    data[0] = val;

    return uda::Vector(data, (size_t)1);
}

uda::Vector atomicVector(const std::string& target, int handle, NTREE* ntree)
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    NTREE* node = findNTreeStructureComponent(logmalloclist, ntree, (char*)target.c_str());
    //NTREE * node = findNTreeStructureComponent(ntree, (char *)target.c_str()); // Locate the named variable target
    if (node == nullptr) {
        return uda::Vector::Null;
    }

    int acount = getNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return uda::Vector::Null; // No atomic data
    }

    char** anames = getNodeAtomicNames(logmalloclist, node);
    char** atypes = getNodeAtomicTypes(logmalloclist, node);
    int* apoint = getNodeAtomicPointers(logmalloclist, node);
    int* arank = getNodeAtomicRank(logmalloclist, node);
    int** ashape = getNodeAtomicShape(logmalloclist, node);

    if (anames == nullptr || atypes == nullptr || apoint == nullptr || arank == nullptr || ashape == nullptr) {
        return uda::Vector::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]) {
            if (std::string("STRING *") == atypes[i] && ((arank[i] == 0 && apoint[i] == 1) || (arank[i] == 1 && apoint[i] == 0))) {
                // String array in a single data structure
                char** val = reinterpret_cast<char**>(getNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));
                return uda::Vector(val, (size_t)ashape[i][0]);
            } else if (arank[i] == 0 && apoint[i] == 1) {
                int count = getNodeStructureComponentDataCount(logmalloclist, node, (char*)target.c_str());
                if (std::string("STRING *") == atypes[i]) return getVector<char*>(logmalloclist, node, target, count);
                if (std::string("char *") == atypes[i]) return getVector<char>(logmalloclist, node, target, count);
                if (std::string("short *") == atypes[i]) return getVector<short>(logmalloclist, node, target, count);
                if (std::string("double *") == atypes[i]) return getVector<double>(logmalloclist, node, target, count);
                if (std::string("float *") == atypes[i]) return getVector<float>(logmalloclist, node, target, count);
                if (std::string("int *") == atypes[i]) return getVector<int>(logmalloclist, node, target, count);
                if (std::string("unsigned int *") == atypes[i]) return getVector<unsigned int>(logmalloclist, node, target, count);
                if (std::string("unsigned short *") == atypes[i]) return getVector<unsigned short>(logmalloclist, node, target, count);
                if (std::string("unsigned char *") == atypes[i]) return getVector<unsigned char>(logmalloclist, node, target, count);
            } else if (arank[i] == 1) {
//                if (std::string("STRING") == atypes[i]) return getVector<char>(logmalloclist, node, target, ashape[i][0]);
//                if (std::string("STRING") == atypes[i]) return getStringVector(logmalloclist, node, target);
                if (std::string("char") == atypes[i]) return getVector<char>(logmalloclist, node, target, ashape[i][0]);
                if (std::string("short") == atypes[i]) return getVector<short>(logmalloclist, node, target, ashape[i][0]);
                if (std::string("double") == atypes[i]) return getVector<double>(logmalloclist, node, target, ashape[i][0]);
                if (std::string("float") == atypes[i]) return getVector<float>(logmalloclist, node, target, ashape[i][0]);
                if (std::string("int") == atypes[i]) return getVector<int>(logmalloclist, node, target, ashape[i][0]);
                if (std::string("unsigned int") == atypes[i]) return getVector<unsigned int>(logmalloclist, node, target, ashape[i][0]);
                if (std::string("unsigned short") == atypes[i]) return getVector<unsigned short>(logmalloclist, node, target, ashape[i][0]);
                if (std::string("unsigned char") == atypes[i]) return getVector<unsigned char>(logmalloclist, node, target, ashape[i][0]);
            } else if (arank[i] == 2 && std::string("STRING") == atypes[i]) {
                return getStringVector(logmalloclist, node, target, ashape[i]);
            }
        }
    }

    return uda::Vector::Null;
}

template <typename T>
static uda::Array getArray(LOGMALLOCLIST* logmalloclist, NTREE* node, const std::string& target, int* shape, int rank)
{
    auto data = reinterpret_cast<T*>(getNodeStructureComponentData(logmalloclist, node, (char*)target.c_str()));

    std::vector<uda::Dim> dims;
    for (int i = 0; i < rank; ++i) {
        std::vector<int> dim((size_t)shape[i]);
        for (int j = 0; j < shape[i]; ++j) {
            dim[j] = j;
        }
        dims.emplace_back(uda::Dim((uda::dim_type)i, dim.data(), (size_t)shape[i], "", ""));
    }

    return uda::Array(data, dims);
}

uda::Array atomicArray(const std::string& target, int handle, NTREE* ntree)
{
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    NTREE* node = findNTreeStructureComponent(logmalloclist, ntree, (char*)target.c_str());
    //NTREE * node = findNTreeStructureComponent(node_, (char *)target.c_str()); // Locate the named variable target
    if (node == nullptr) {
        return uda::Array::Null;
    }

    int acount = getNodeAtomicCount(node); // Number of atomic typed structure members
    if (acount == 0) {
        return uda::Array::Null; // No atomic data
    }

    char** anames = getNodeAtomicNames(logmalloclist, node);
    char** atypes = getNodeAtomicTypes(logmalloclist, node);
    int* apoint = getNodeAtomicPointers(logmalloclist, node);
    int* arank = getNodeAtomicRank(logmalloclist, node);
    int** ashape = getNodeAtomicShape(logmalloclist, node);

    if (anames == nullptr || atypes == nullptr || apoint == nullptr || arank == nullptr || ashape == nullptr) {
        return uda::Array::Null;
    }

    for (int i = 0; i < acount; i++) {
        if (target == anames[i]) {
            if (std::string("STRING") == atypes[i]) return getArray<char*>(logmalloclist, node, target, ashape[i], arank[i]);
            if (std::string("short") == atypes[i]) return getArray<short>(logmalloclist, node, target, ashape[i], arank[i]);
            if (std::string("double") == atypes[i]) return getArray<double>(logmalloclist, node, target, ashape[i], arank[i]);
            if (std::string("float") == atypes[i]) return getArray<float>(logmalloclist, node, target, ashape[i], arank[i]);
            if (std::string("int") == atypes[i]) return getArray<int>(logmalloclist, node, target, ashape[i], arank[i]);
            if (std::string("unsigned int") == atypes[i]) return getArray<unsigned int>(logmalloclist, node, target, ashape[i], arank[i]);
            if (std::string("unsigned short") == atypes[i]) return getArray<unsigned short>(logmalloclist, node, target, ashape[i], arank[i]);
            if (std::string("unsigned char") == atypes[i]) return getArray<unsigned char>(logmalloclist, node, target, ashape[i], arank[i]);
        }
    }

    return uda::Array::Null;
}

TEST_CASE( "Test help function", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::help()", "");

    REQUIRE( handle >= 0 );
    
    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    
    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_STRING );

    auto str = std::string{ data };

    std::string expected = "\nTestplugin: Functions Names and Test Descriptions/n/n"
            "test0-test9: String passing tests\n"
            "\ttest0: single string as a char array\n"
            "\ttest1: single string\n"
            "\ttest2: multiple strings as a 2D array of chars\n"
            "\ttest3: array of strings\n"
            "\ttest4: data structure with a fixed length single string\n"
            "\ttest5: data structure with a fixed length multiple string\n"
            "\ttest6: data structure with an arbitrary length single string\n"
            "\ttest7: data structure with a fixed number of arbitrary length strings\n"
            "\ttest8: data structure with an arbitrary number of arbitrary length strings\n\n"
            "\ttest9: array of data structures with a variety of string types\n\n"
            "\ttest9A: array of data structures with a variety of string types and single sub structure\n\n"

            "***test10-test18: Integer passing tests\n"
            "\ttest10: single integer\n"
            "\ttest11: fixed number (rank 1 array) of integers\n"
            "\ttest12: arbitrary number (rank 1 array) of integers\n"
            "\ttest13: fixed length rank 2 array of integers\n"
            "\ttest14: arbitrary length rank 2 array of integers\n"
            "\ttest15: data structure with a single integer\n"
            "\ttest16: data structure with a fixed number of integers\n"
            "\ttest17: data structure with a arbitrary number of integers\n"
            "\ttest18: array of data structures with a variety of integer types\n\n"

            "***test20-test28: Short Integer passing tests\n"
            "\ttest20: single integer\n"
            "\ttest21: fixed number (rank 1 array) of integers\n"
            "\ttest22: arbitrary number (rank 1 array) of integers\n"
            "\ttest23: fixed length rank 2 array of integers\n"
            "\ttest24: arbitrary length rank 2 array of integers\n"
            "\ttest25: data structure with a single integer\n"
            "\ttest26: data structure with a fixed number of integers\n"
            "\ttest27: data structure with a arbitrary number of integers\n"
            "\ttest28: array of data structures with a variety of integer types\n\n"

            "***test30-test32: double passing tests\n"
            "\ttest30: pair of doubles (Coordinate)\n"

            "***test40-test40: put data block receiving tests\n"
            "\ttest50: Passing parameters into plugins via the source argument\n"
            "\ttest60-62: ENUMLIST structures\n\n"

            "plugin: test calling other plugins\n"

            "error: Error reporting and server termination tests\n";

    REQUIRE( str == expected );
}

TEST_CASE( "Run test0 - pass string as char array", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test0()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_CHAR );

    auto str = std::string{ data };

    std::string expected = "Hello World!";

    REQUIRE( str == expected );
}

TEST_CASE( "Run test1 - pass string as string scalar", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test1()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_STRING );

    auto str = std::string{ data };

    std::string expected = "Hello World!";

    REQUIRE( str == expected );
}

TEST_CASE( "Run test2 - pass string list as 2D char array", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test2()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_CHAR );

    REQUIRE( udaGetRank(handle) == 2 );

    REQUIRE( udaGetDimNum(handle, 0) == 16 );
    REQUIRE( udaGetDimNum(handle, 1) == 3 );

    std::vector<std::string> strings;
    strings.emplace_back(std::string(data + 0 * 16, strlen(data + 0 * 16)));
    strings.emplace_back(std::string(data + 1 * 16, strlen(data + 1 * 16)));
    strings.emplace_back(std::string(data + 2 * 16, strlen(data + 2 * 16)));

    std::vector<std::string> expected;
    expected.emplace_back("Hello World!");
    expected.emplace_back("Qwerty keyboard");
    expected.emplace_back("MAST Upgrade");

    REQUIRE( strings == expected );
}

TEST_CASE( "Run test3 - pass string list as array of strings", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test3()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_STRING );

    REQUIRE( udaGetRank(handle) == 2 );
    REQUIRE( udaGetDimNum(handle, 0) == 16 );
    REQUIRE( udaGetDimNum(handle, 1) == 3 );

    std::vector<std::string> strings;
    strings.emplace_back(std::string(data + 0 * 16, strlen(data + 0 * 16)));
    strings.emplace_back(std::string(data + 1 * 16, strlen(data + 1 * 16)));
    strings.emplace_back(std::string(data + 2 * 16, strlen(data + 2 * 16)));

    std::vector<std::string> expected;
    expected.emplace_back("Hello World!");
    expected.emplace_back("Qwerty keyboard");
    expected.emplace_back("MAST Upgrade");

    REQUIRE( strings == expected );
}

TEST_CASE( "Run test4 - pass struct containing char array", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test4()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    REQUIRE( udaGetDataOpaqueType(handle) == UDA_OPAQUE_TYPE_STRUCTURES );

    REQUIRE( udaSetDataTree(handle) == 1 );

    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    getNodeChildrenCount(ntree);
    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    
    USERDEFINEDTYPELIST* userdefinedtypelist = udaGetUserDefinedTypeList(handle);
    USERDEFINEDTYPE* type = getNodeUserDefinedType(ntree);
    printUserDefinedTypeTable(userdefinedtypelist, *type);

    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    printNTreeStructureNames(logmalloclist, ntree);
    
    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );

    uda::Scalar value = atomicScalar("value", handle, ntree);

    REQUIRE( value.type().name() == typeid(char*).name() );

    std::string str(value.as<char*>());
    REQUIRE( str == "012345678901234567890" );
}

TEST_CASE( "Run test5 - pass struct containing array of strings", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test5()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);

    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    
    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "STRING" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 2 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 56 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][1] == 3 );

    uda::Vector value = atomicVector("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(char*).name() );
    REQUIRE( value.size() == 3 );

    std::vector<char*> vec = value.as<char*>();

    REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
    REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
    REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
}

TEST_CASE( "Run test6 - pass struct containing string", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test6()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);

    USERDEFINEDTYPELIST* userdefinedtypelist = udaGetUserDefinedTypeList(handle);
    USERDEFINEDTYPE* type = getNodeUserDefinedType(ntree);
    printUserDefinedTypeTable(userdefinedtypelist, *type);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    printNTreeStructureNames(logmalloclist, ntree);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );

    uda::Scalar value = atomicScalar("value", handle, ntree);

    REQUIRE( value.type().name() == typeid(char*).name() );

    std::string str(value.as<char*>());
    REQUIRE( str == "PI=3.1415927" );
}

TEST_CASE( "Run test7 - pass struct containing array of strings", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test7()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "STRING *" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 1 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 3 );

    uda::Vector value = atomicVector("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(char*).name() );
    REQUIRE( value.size() == 3 );

    std::vector<char*> vec = value.as<char*>();

    REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
    REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
    REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
}

TEST_CASE( "Run test8 - pass struct containing array of string pointers", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test8()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == true );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "STRING *" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );

    uda::Vector value = atomicVector("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(char*).name() );
    REQUIRE( value.size() == 3 );

    std::vector<char*> vec = value.as<char*>();

    REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
    REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
    REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
}

TEST_CASE( "Run test9 - pass 4 structs containing multiple types of string arrays", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test9()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 4 );

    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    
    for (int i = 0; i < 4; ++i)
    {
        NTREE* child = getNodeChild(ntree, i);

        REQUIRE( std::string{ getNodeStructureName(child) } == "data" );
        REQUIRE( getNodeChildrenCount(child) == 0 );
        REQUIRE( getNodeAtomicCount(child) == 5 );

        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, child)[0] } == "v1" );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, child)[1] } == "v2" );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, child)[2] } == "v3" );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, child)[3] } == "v4" );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, child)[4] } == "v5" );

        REQUIRE( getNodeAtomicPointers(logmalloclist, child)[0] == false );
        REQUIRE( getNodeAtomicPointers(logmalloclist, child)[1] == false );
        REQUIRE( getNodeAtomicPointers(logmalloclist, child)[2] == true );
        REQUIRE( getNodeAtomicPointers(logmalloclist, child)[3] == false );
        REQUIRE( getNodeAtomicPointers(logmalloclist, child)[4] == true );

        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, child)[0] } == "STRING" );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, child)[1] } == "STRING" );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, child)[2] } == "STRING" );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, child)[3] } == "STRING *" );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, child)[4] } == "STRING *" );
        
        REQUIRE( getNodeAtomicRank(logmalloclist, child)[0] == 1 );
        REQUIRE( getNodeAtomicRank(logmalloclist, child)[1] == 2 );
        REQUIRE( getNodeAtomicRank(logmalloclist, child)[2] == 0 );
        REQUIRE( getNodeAtomicRank(logmalloclist, child)[3] == 1 );
        REQUIRE( getNodeAtomicRank(logmalloclist, child)[4] == 0 );

        // v1
        {
            uda::Scalar value = atomicScalar("v1", handle, child);
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( std::string(value.as<char*>()) == "123212321232123212321" );
        }

        // v2
        {
            uda::Vector value = atomicVector("v2", handle, child);
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( value.size() == 3 );

            std::vector<char*> vec = value.as<char*>();

            REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
            REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
            REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
        }

        // v3
        {
            uda::Scalar value = atomicScalar("v3", handle, child);
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( std::string(value.as<char*>()) == "PI=3.1415927" );
        }

        // v4
        {
            uda::Vector value = atomicVector("v4", handle, child);
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( value.size() == 3 );

            std::vector<char*> vec = value.as<char*>();

            REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
            REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
            REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
        }

        // v5
        {
            uda::Vector value = atomicVector("v5", handle, child);
            REQUIRE( !value.isNull() );

            REQUIRE( value.type().name() == typeid(char*).name() );
            REQUIRE( value.size() == 3 );

            std::vector<char*> vec = value.as<char*>();

            REQUIRE( std::string(vec.at(0)) == "012345678901234567890" );
            REQUIRE( std::string(vec.at(1)) == "QWERTY KEYBOARD" );
            REQUIRE( std::string(vec.at(2)) == "MAST TOKAMAK" );
        }
    }
}

TEST_CASE( "Run test10 - pass single int", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test10()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_INT );

    REQUIRE( *(int*)udaGetData(handle) == 7 );
}

TEST_CASE( "Run test11 - pass struct containing single int", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test11()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );

    uda::Scalar value = atomicScalar("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 0 );

    REQUIRE( value.as<int>() == 11 );
}

TEST_CASE( "Run test12 - pass struct containing 1D array of ints", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test12()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 1 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 3 );

    uda::Vector value = atomicVector("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 3 );

    std::vector<int> expected;
    expected.emplace_back(10);
    expected.emplace_back(11);
    expected.emplace_back(12);

    REQUIRE( value.as<int>() == expected );
}

TEST_CASE( "Run test13 - pass struct containing 2D array of ints", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test13()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 2 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 2 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][1] == 3 );

    uda::Array value = atomicArray("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 6 );

    REQUIRE( value.dims().size() == 2 );
    REQUIRE( value.dims()[0].size() == 2 );
    REQUIRE( value.dims()[1].size() == 3 );

    std::vector<int> expected;
    expected.emplace_back(0);
    expected.emplace_back(1);
    expected.emplace_back(2);
    expected.emplace_back(10);
    expected.emplace_back(11);
    expected.emplace_back(12);

    REQUIRE( value.as<int>() == expected );
}

TEST_CASE( "Run test14 - pass struct containing single int passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test14()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == true );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );

    uda::Scalar value = atomicScalar("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 0 );

    REQUIRE( value.as<int>() == 14 );
}

TEST_CASE( "Run test15 - pass struct containing 1D array of ints passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test15()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == true );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 1 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 3 );

    uda::Vector value = atomicVector("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 3 );

    std::vector<int> expected;
    expected.emplace_back(13);
    expected.emplace_back(14);
    expected.emplace_back(15);

    REQUIRE( value.as<int>() == expected );
}

TEST_CASE( "Run test16 - pass struct containing 2D array of ints passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test16()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == true );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 2 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 2 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][1] == 3 );

    uda::Array value = atomicArray("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(int).name() );
    REQUIRE( value.size() == 6 );

    REQUIRE( value.dims().size() == 2 );
    REQUIRE( value.dims()[0].size() == 2 );
    REQUIRE( value.dims()[1].size() == 3 );

    std::vector<int> expected;
    expected.emplace_back(0);
    expected.emplace_back(1);
    expected.emplace_back(2);
    expected.emplace_back(10);
    expected.emplace_back(11);
    expected.emplace_back(12);

    REQUIRE( value.as<int>() == expected );
}

TEST_CASE( "Run test18 - pass large number of structs containing single int", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test18()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 100000 );

    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    
    for (int i = 0; i < 100000; ++i) {
        NTREE* child = getNodeChild(ntree, i);

        REQUIRE( std::string{ getNodeStructureName(child) } == "data" );
        REQUIRE( getNodeChildrenCount(child) == 0 );
        REQUIRE( getNodeAtomicCount(child) == 1 );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, child)[0] } == "value" );
        REQUIRE( getNodeAtomicPointers(logmalloclist, child)[0] == false );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, child)[0] } == "int" );
        REQUIRE( getNodeAtomicRank(logmalloclist, child)[0] == 0 );

        uda::Scalar value = atomicScalar("value", handle, child);
        REQUIRE( !value.isNull() );

        REQUIRE( value.type().name() == typeid(int).name() );
        REQUIRE( value.size() == 0 );

        REQUIRE( value.as<int>() == i );
    }
}

TEST_CASE( "Run test19 - pass 3 structs containing array of structs", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test19()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 3 );

    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    
    for (int i = 0; i < 3; ++i) {
        NTREE* child = getNodeChild(ntree, i);

        REQUIRE(std::string{ getNodeStructureName(child) } == "data");
        REQUIRE(getNodeChildrenCount(child) == 7);
        REQUIRE(getNodeAtomicCount(child) == 1);
        REQUIRE(std::string{ getNodeAtomicNames(logmalloclist, child)[0] } == "value");
        REQUIRE(getNodeAtomicPointers(logmalloclist, child)[0] == false);
        REQUIRE(std::string{ getNodeAtomicTypes(logmalloclist, child)[0] } == "int");
        REQUIRE(getNodeAtomicRank(logmalloclist, child)[0] == 0);

        uda::Scalar value = atomicScalar("value", handle, child);
        REQUIRE(!value.isNull());

        REQUIRE(value.type().name() == typeid(int).name());
        REQUIRE(value.size() == 0);

        REQUIRE(value.as<int>() == 3 + i);

        for (int j = 0; j < 7; ++j) {
            NTREE* subchild = getNodeChild(child, j);

            REQUIRE(std::string{ getNodeStructureName(subchild) } == "vals");
            REQUIRE(getNodeChildrenCount(subchild) == 0);
            REQUIRE(getNodeAtomicCount(subchild) == 1);
            REQUIRE(std::string{ getNodeAtomicNames(logmalloclist, subchild)[0] } == "value");
            REQUIRE(getNodeAtomicPointers(logmalloclist, subchild)[0] == false);
            REQUIRE(std::string{ getNodeAtomicTypes(logmalloclist, subchild)[0] } == "int");
            REQUIRE(getNodeAtomicRank(logmalloclist, subchild)[0] == 0);

            value = atomicScalar("value", handle, subchild);
            REQUIRE(!value.isNull());

            REQUIRE(value.type().name() == typeid(int).name());
            REQUIRE(value.size() == 0);

            REQUIRE(value.as<int>() == 10 * i + j);
        }
    }
}

TEST_CASE( "Run test20 - pass single short", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test20()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_SHORT );

    REQUIRE( *(short*)udaGetData(handle) == 7 );
}

TEST_CASE( "Run test21 - pass struct containing single short", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test21()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "short" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );

    uda::Scalar value = atomicScalar("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 0 );

    REQUIRE( value.as<short>() == 21 );
}

TEST_CASE( "Run test22 - pass struct containing 1D array of shorts", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test22()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "short" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 1 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 3 );

    uda::Vector value = atomicVector("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 3 );

    std::vector<short> expected;
    expected.emplace_back(20);
    expected.emplace_back(21);
    expected.emplace_back(22);

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test23 - pass struct containing 2D array of shorts", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test23()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "short" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 2 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 3 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][1] == 2 );

    uda::Array value = atomicArray("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 6 );

    REQUIRE( value.dims().size() == 2 );
    REQUIRE( value.dims()[0].size() == 3 );
    REQUIRE( value.dims()[1].size() == 2 );

    std::vector<short> expected;
    expected.emplace_back(0);
    expected.emplace_back(1);
    expected.emplace_back(2);
    expected.emplace_back(10);
    expected.emplace_back(11);
    expected.emplace_back(12);

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test24 - pass struct containing single short passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test24()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == true );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "short" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );

    uda::Scalar value = atomicScalar("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 0 );

    REQUIRE( value.as<short>() == 24 );
}

TEST_CASE( "Run test25 - pass struct containing 1D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test25()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == true );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "short" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 1 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 3 );

    uda::Vector value = atomicVector("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 3 );

    std::vector<short> expected;
    expected.emplace_back(13);
    expected.emplace_back(14);
    expected.emplace_back(15);

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test26 - pass struct containing 2D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test26()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == true );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "short" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 2 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 3 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][1] == 2 );

    uda::Array value = atomicArray("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 6 );

    REQUIRE( value.dims().size() == 2 );
    REQUIRE( value.dims()[0].size() == 3 );
    REQUIRE( value.dims()[1].size() == 2 );

    std::vector<short> expected;
    expected.emplace_back(13);
    expected.emplace_back(14);
    expected.emplace_back(15);
    expected.emplace_back(23);
    expected.emplace_back(24);
    expected.emplace_back(25);

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test27 - pass struct containing 3D array of shorts", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test27()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "short" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 3 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 4 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][1] == 3 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][2] == 2 );

    uda::Array value = atomicArray("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 24 );

    REQUIRE( value.dims().size() == 3 );
    REQUIRE( value.dims()[0].size() == 4 );
    REQUIRE( value.dims()[1].size() == 3 );
    REQUIRE( value.dims()[2].size() == 2 );

    short exp[] = { 0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 100, 101, 102, 103, 110, 111,
                    112, 113, 120, 121, 122, 123 };
    std::vector<short> expected(exp, exp + sizeof(exp)/sizeof(exp[0]));

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test28 - pass struct containing 3D array of shorts passed as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test28()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "value" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == true );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "short" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 3 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][0] == 4 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][1] == 3 );
    REQUIRE( getNodeAtomicShape(logmalloclist, ntree)[0][2] == 2 );

    uda::Array value = atomicArray("value", handle, ntree);
    REQUIRE( !value.isNull() );

    REQUIRE( value.type().name() == typeid(short).name() );
    REQUIRE( value.size() == 24 );

    REQUIRE( value.dims().size() == 3 );
    REQUIRE( value.dims()[0].size() == 4 );
    REQUIRE( value.dims()[1].size() == 3 );
    REQUIRE( value.dims()[2].size() == 2 );

    short exp[] = { 0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 100, 101, 102, 103, 110, 111,
                    112, 113, 120, 121, 122, 123 };
    std::vector<short> expected(exp, exp + sizeof(exp)/sizeof(exp[0]));

    REQUIRE( value.as<short>() == expected );
}

TEST_CASE( "Run test30 - pass struct containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test30()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 0 );
    REQUIRE( getNodeAtomicCount(ntree) == 2 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "R" );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[1] } == "Z" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[1] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "double" );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[1] } == "double" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[1] == 0 );

    uda::Scalar R = atomicScalar("R", handle, ntree);
    REQUIRE( !R.isNull() );

    REQUIRE( R.type().name() == typeid(double).name() );
    REQUIRE( R.as<double>() == Approx(1.0) );

    uda::Scalar Z = atomicScalar("Z", handle, ntree);
    REQUIRE( !Z.isNull() );

    REQUIRE( Z.type().name() == typeid(double).name() );
    REQUIRE( Z.as<double>() == Approx(2.0) );
}

TEST_CASE( "Run test31 - pass 100 structs containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test31()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 100 );

    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);
    
    for (int i = 0; i < 100; ++i) {
        NTREE* child = getNodeChild(ntree, i);

        REQUIRE( std::string{ getNodeStructureName(child) } == "data" );
        REQUIRE( getNodeChildrenCount(child) == 0 );
        REQUIRE( getNodeAtomicCount(child) == 2 );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, child)[0] } == "R" );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, child)[1] } == "Z" );
        REQUIRE( getNodeAtomicPointers(logmalloclist, child)[0] == false );
        REQUIRE( getNodeAtomicPointers(logmalloclist, child)[1] == false );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, child)[0] } == "double" );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, child)[1] } == "double" );
        REQUIRE( getNodeAtomicRank(logmalloclist, child)[0] == 0 );
        REQUIRE( getNodeAtomicRank(logmalloclist, child)[1] == 0 );

        uda::Scalar R = atomicScalar("R", handle, child);
        REQUIRE( !R.isNull() );

        REQUIRE( R.type().name() == typeid(double).name() );
        REQUIRE( R.as<double>() == Approx(1.0 * i) );

        uda::Scalar Z = atomicScalar("Z", handle, child);
        REQUIRE( !Z.isNull() );

        REQUIRE( Z.type().name() == typeid(double).name() );
        REQUIRE( Z.as<double>() == Approx(10.0 * i) );
    }
}

TEST_CASE( "Run test32 - pass struct containing array of 100 structs containing 2 doubles", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test32()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 100 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "count" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );

    uda::Scalar count = atomicScalar("count", handle, ntree);
    REQUIRE( !count.isNull() );

    REQUIRE( count.type().name() == typeid(int).name() );
    REQUIRE( count.as<int>() == 100 );

    for (int i = 0; i < 100; ++i) {
        NTREE* coord = getNodeChild(ntree, i);

        REQUIRE( std::string{ getNodeStructureName(coord) } == "coords" );
        REQUIRE( getNodeChildrenCount(coord) == 0 );
        REQUIRE( getNodeAtomicCount(coord) == 2 );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, coord)[0] } == "R" );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, coord)[1] } == "Z" );
        REQUIRE( getNodeAtomicPointers(logmalloclist, coord)[0] == false );
        REQUIRE( getNodeAtomicPointers(logmalloclist, coord)[1] == false );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, coord)[0] } == "double" );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, coord)[1] } == "double" );
        REQUIRE( getNodeAtomicRank(logmalloclist, coord)[0] == 0 );
        REQUIRE( getNodeAtomicRank(logmalloclist, coord)[1] == 0 );

        uda::Scalar R = atomicScalar("R", handle, coord);
        REQUIRE( !R.isNull() );

        REQUIRE( R.type().name() == typeid(double).name() );
        REQUIRE( R.as<double>() == Approx(1.0 * i) );

        uda::Scalar Z = atomicScalar("Z", handle, coord);
        REQUIRE( !Z.isNull() );

        REQUIRE( Z.type().name() == typeid(double).name() );
        REQUIRE( Z.as<double>() == Approx(10.0 * i) );
    }
}

TEST_CASE( "Run test33 - pass struct containing array of 100 structs containing 2 doubles as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test33()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 100 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "count" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );

    uda::Scalar count = atomicScalar("count", handle, ntree);
    REQUIRE( !count.isNull() );

    REQUIRE( count.type().name() == typeid(int).name() );
    REQUIRE( count.as<int>() == 100 );

    for (int i = 0; i < 100; ++i) {
        NTREE* coord = getNodeChild(ntree, i);

        REQUIRE( std::string{ getNodeStructureName(coord) } == "coords" );
        REQUIRE( getNodeChildrenCount(coord) == 0 );
        REQUIRE( getNodeAtomicCount(coord) == 2 );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, coord)[0] } == "R" );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, coord)[1] } == "Z" );
        REQUIRE( getNodeAtomicPointers(logmalloclist, coord)[0] == false );
        REQUIRE( getNodeAtomicPointers(logmalloclist, coord)[1] == false );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, coord)[0] } == "double" );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, coord)[1] } == "double" );
        REQUIRE( getNodeAtomicRank(logmalloclist, coord)[0] == 0 );
        REQUIRE( getNodeAtomicRank(logmalloclist, coord)[1] == 0 );

        uda::Scalar R = atomicScalar("R", handle, coord);
        REQUIRE( !R.isNull() );

        REQUIRE( R.type().name() == typeid(double).name() );
        REQUIRE( R.as<double>() == Approx(1.0 * i) );

        uda::Scalar Z = atomicScalar("Z", handle, coord);
        REQUIRE( !Z.isNull() );

        REQUIRE( Z.type().name() == typeid(double).name() );
        REQUIRE( Z.as<double>() == Approx(10.0 * i) );
    }
}

TEST_CASE( "Run test34 - pass struct containing array of 100 structs containing 2 doubles as pointer", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::test34()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );
    NTREE* ntree = udaGetDataTree(handle);
    REQUIRE( ntree != nullptr );

    REQUIRE( udaSetDataTree(handle) == 1 );

    REQUIRE( getNodeChildrenCount(ntree) == 1 );

    ntree = getNodeChild(ntree, 0);
    LOGMALLOCLIST* logmalloclist = udaGetLogMallocList(handle);

    REQUIRE( std::string{ getNodeStructureName(ntree) } == "data" );
    REQUIRE( getNodeChildrenCount(ntree) == 100 );
    REQUIRE( getNodeAtomicCount(ntree) == 1 );
    REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, ntree)[0] } == "count" );
    REQUIRE( getNodeAtomicPointers(logmalloclist, ntree)[0] == false );
    REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, ntree)[0] } == "int" );
    REQUIRE( getNodeAtomicRank(logmalloclist, ntree)[0] == 0 );

    uda::Scalar count = atomicScalar("count", handle, ntree);
    REQUIRE( !count.isNull() );

    REQUIRE( count.type().name() == typeid(int).name() );
    REQUIRE( count.as<int>() == 100 );

    for (int i = 0; i < 100; ++i) {
        NTREE* coord = getNodeChild(ntree, i);

        REQUIRE( std::string{ getNodeStructureName(coord) } == "coords" );
        REQUIRE( getNodeChildrenCount(coord) == 0 );
        REQUIRE( getNodeAtomicCount(coord) == 2 );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, coord)[0] } == "R" );
        REQUIRE( std::string{ getNodeAtomicNames(logmalloclist, coord)[1] } == "Z" );
        REQUIRE( getNodeAtomicPointers(logmalloclist, coord)[0] == true );
        REQUIRE( getNodeAtomicPointers(logmalloclist, coord)[1] == true );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, coord)[0] } == "unsigned char *" );
        REQUIRE( std::string{ getNodeAtomicTypes(logmalloclist, coord)[1] } == "unsigned char *" );
        REQUIRE( getNodeAtomicRank(logmalloclist, coord)[0] == 0 );
        REQUIRE( getNodeAtomicRank(logmalloclist, coord)[1] == 0 );
        REQUIRE( getNodeAtomicRank(logmalloclist, coord)[0] == 0 );
        REQUIRE( getNodeAtomicRank(logmalloclist, coord)[1] == 0 );
        REQUIRE( getNodeAtomicShape(logmalloclist, coord)[0][0] == 10 );
        REQUIRE( getNodeAtomicShape(logmalloclist, coord)[1][0] == 10 );

        uda::Vector R = atomicVector("R", handle, coord);
        REQUIRE( !R.isNull() );

        REQUIRE( R.type().name() == typeid(unsigned char).name() );
//        std::vector<char> exp = { 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i, 1.0 * i };
        unsigned char val = 1 * i;
        std::vector<unsigned char> exp = { val, val, val, val, val, val, val, val, val, val };
        REQUIRE( R.as<unsigned char>() == exp );

        uda::Vector Z = atomicVector("Z", handle, coord);
        REQUIRE( !Z.isNull() );

        REQUIRE( Z.type().name() == typeid(unsigned char).name() );
//        exp = { 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i, 10.0 * i };
        val = 10 * i;
        exp = { val, val, val, val, val, val, val, val, val, val };
        REQUIRE( Z.as<unsigned char>() == exp );
    }
}

TEST_CASE( "Run plugin - call a plugin", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::plugin(signal='HELP::HELP()', source='')", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_STRING );

    auto str = std::string{ data };

    std::string expected = "\n"
            "Help\tList of HELP plugin functions:\n"
            "\n"
            "services()\tReturns a list of available services with descriptions\n"
            "ping()\t\tReturn the Local Server Time in seconds and microseonds\n"
            "servertime()\tReturn the Local Server Time in seconds and microseonds\n\n";

    REQUIRE( str == expected );
}

TEST_CASE( "Run scalartest - return a simple scalar value", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::scalartest()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data != nullptr );
    REQUIRE( type == UDA_TYPE_INT );

    REQUIRE( *(int*)udaGetData(handle) == 10 );
}

TEST_CASE( "Run emptytest - return no data", "[plugins][TESTPLUGIN]" )
{
#include "setup.inc"

    int handle = udaGetAPI("TESTPLUGIN::emptytest()", "");

    REQUIRE( udaGetErrorCode(handle) == 0 );
    REQUIRE( std::string{ udaGetErrorMsg(handle) }.empty() );

    const char* data = udaGetData(handle);
    int type = udaGetDataType(handle);

    REQUIRE( data == nullptr );
}
