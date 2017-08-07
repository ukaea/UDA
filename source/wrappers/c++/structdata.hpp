//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_STRUCTDATA_H
#define IDAM_WRAPPERS_CPP_STRUCTDATA_H

#include <vector>
#include <string>
#include <cxxabi.h>

namespace uda {

class StructData
{
public:
    explicit StructData(bool isnull = false) : isnull_(isnull) {}

    template <typename T>
    std::vector<T *> as()
    {
        int status;
        std::string tname = typeid(T).name();
        std::string demangled_name(abi::__cxa_demangle(tname.c_str(), nullptr, nullptr, &status));

        std::vector<T *> vec;
        for (const auto& el : data_) {
            if ((sizeof(T) == el.size_) & (demangled_name == el.name_)) {
                vec.push_back(reinterpret_cast<T *>(el.ptr_));
            }
        }
        return vec;
    }

    void append(std::string name, size_t size, void * ptr)
    {
        data_.emplace_back(_Struct(std::move(name), size, ptr));
    }

    bool isNull() const { return isnull_; }
    static StructData Null;
private:
    struct _Struct
    {
        _Struct(std::string name, size_t size, void * ptr) : name_(std::move(name)), size_(size), ptr_(ptr) {}
        std::string name_;
        size_t size_;
        void * ptr_;
    };
    std::vector<_Struct> data_;
    bool isnull_;
};

}

#endif //IDAM_WRAPPERS_CPP_STRUCTDATA_H
