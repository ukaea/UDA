//
// Created by jholloc on 08/03/16.
//

#ifndef IDAM_WRAPPERS_CPP_RESULT_H
#define IDAM_WRAPPERS_CPP_RESULT_H

#include <typeinfo>
#include <string>
#include <boost/noncopyable.hpp>
#include <map>

#include "dim.hpp"
#include "treenode.hpp"

namespace uda {

class Data;

class Result : private boost::noncopyable {
public:
    ~Result();

    const std::string error() const;
    int errorCode() const;

    Data* data() const;

    const std::type_info& type() const
    { return *type_; };

    const std::string& label() const
    { return label_; }

    const std::string& units() const
    { return units_; }

    const std::string& description() const
    { return desc_; }

    size_t size() const
    { return size_; }

    dim_type rank() const
    { return rank_; }

    const std::map<std::string, std::string>& meta()
    { return meta_; }

    Dim dim(dim_type ndim) const;

    bool isTree() const
    { return istree_; }

    TreeNode tree() const;

private:
    Result(int handle);

    friend class Client;

    template <typename T>
    T as()
    { return boost::any_cast<T>(data_); }

    const int handle_;
    std::string label_;
    std::string units_;
    std::string desc_;
    const std::type_info* type_;
    boost::any data_;
    dim_type rank_;
    size_t size_;
    const std::vector<dim_type> dims_;
    bool istree_;
    std::map<std::string, std::string> meta_;
};

}

#endif //IDAM_WRAPPERS_CPP_RESULT_H

