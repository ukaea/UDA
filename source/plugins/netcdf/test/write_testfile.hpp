#pragma once
#include <string>
#include "random_generator.hpp"

namespace nc_test_files
{
    template<typename T> struct TypeCompound
    {
        T data[10];
        char* description;
        char c;
    };

    void create_testfile_folder(std::string name);
    void write_test_files(std::string file_path);
}

template struct nc_test_files::TypeCompound<int>;
template struct nc_test_files::TypeCompound<char>;
template struct nc_test_files::TypeCompound<float>;
template struct nc_test_files::TypeCompound<double>;
template struct nc_test_files::TypeCompound<long long>;
template struct nc_test_files::TypeCompound<short>;
template struct nc_test_files::TypeCompound<unsigned int>;
template struct nc_test_files::TypeCompound<unsigned short>;
template struct nc_test_files::TypeCompound<unsigned long long>;

template struct nc_test_files::TypeCompound<int8_t>;
// template struct nc_test_files::TypeCompound<int16_t>;
// template struct nc_test_files::TypeCompound<int32_t>;
// template struct nc_test_files::TypeCompound<int64_t>;
template struct nc_test_files::TypeCompound<uint8_t>;
// template struct nc_test_files::TypeCompound<uint16_t>;
// template struct nc_test_files::TypeCompound<uint32_t>;
// template struct nc_test_files::TypeCompound<uint64_t>;

