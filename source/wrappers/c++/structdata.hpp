//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_STRUCTDATA_H
#define IDAM_WRAPPERS_CPP_STRUCTDATA_H

#include <vector>
#include <string>
#include <cxxabi.h>

namespace Idam {

class StructData
{
public:
    StructData(bool isnull = false) : data_(), isnull_(isnull) {}
    template <typename T>
    std::vector<T *> as()
    {
        int status;
        std::string tname = typeid(T).name();
        std::string demangled_name(abi::__cxa_demangle(tname.c_str(), NULL, NULL, &status));

        std::vector<T *> vec;
        for (size_t i = 0; i < data_.size(); ++i) {
            if ((sizeof(T) == data_[i].size_) & (demangled_name == data_[i].name_)) {
                vec.push_back(reinterpret_cast<T *>(data_[i].ptr_));
            }
        }
        return vec;
    }
    void append(std::string name, size_t size, void * ptr)
    {
        data_.push_back(_Struct(name, size, ptr));
    }
    bool isNull() const { return isnull_; }
    static StructData Null;
private:
    struct _Struct
    {
        _Struct(std::string name, size_t size, void * ptr) : name_(name), size_(size), ptr_(ptr) {}
        std::string name_;
        size_t size_;
        void * ptr_;
    };
    std::vector<_Struct> data_;
    bool isnull_;
};

}

#endif //IDAM_WRAPPERS_CPP_STRUCTDATA_H
