#pragma once

template <typename T>
struct TypeConvertor
{
    static UdaType type;
};

template <typename T>
UdaType TypeConvertor<T>::type = UdaType::UDA_TYPE_UNKNOWN;

template <> UdaType TypeConvertor<char>::type = UDA_TYPE_CHAR;
template <> UdaType TypeConvertor<short>::type = UDA_TYPE_SHORT;
template <> UdaType TypeConvertor<int>::type = UDA_TYPE_INT;
template <> UdaType TypeConvertor<unsigned int>::type = UDA_TYPE_UNSIGNED_INT;
template <> UdaType TypeConvertor<long>::type = UDA_TYPE_LONG;
template <> UdaType TypeConvertor<float>::type = UDA_TYPE_FLOAT;
template <> UdaType TypeConvertor<double>::type = UDA_TYPE_DOUBLE;
template <> UdaType TypeConvertor<unsigned char>::type = UDA_TYPE_UNSIGNED_CHAR;
template <> UdaType TypeConvertor<unsigned short>::type = UDA_TYPE_UNSIGNED_SHORT;
template <> UdaType TypeConvertor<unsigned long>::type = UDA_TYPE_UNSIGNED_LONG;
template <> UdaType TypeConvertor<long long>::type = UDA_TYPE_LONG64;
template <> UdaType TypeConvertor<unsigned long long>::type = UDA_TYPE_UNSIGNED_LONG64;
template <> UdaType TypeConvertor<float _Complex>::type = UDA_TYPE_COMPLEX;
template <> UdaType TypeConvertor<double _Complex>::type = UDA_TYPE_DCOMPLEX;
template <> UdaType TypeConvertor<void>::type = UDA_TYPE_UNDEFINED;
template <> UdaType TypeConvertor<const char*>::type = UDA_TYPE_STRING;
