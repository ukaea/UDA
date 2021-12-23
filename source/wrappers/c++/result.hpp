#ifndef UDA_WRAPPERS_CPP_RESULT_H
#define UDA_WRAPPERS_CPP_RESULT_H

#include <typeinfo>
#include <string>
#include <boost/noncopyable.hpp>
#include <map>

#include <clientserver/export.h>
#include "dim.hpp"
#include "treenode.hpp"

#if defined(_WIN32)
#  if !defined(__GNUC__)
#    pragma warning(push)
#    pragma warning(disable: 4251)
#    pragma warning(disable: 4275)
#  endif
#endif

namespace uda {

class Data;

class LIBRARY_API Result : private boost::noncopyable {
public:
    enum DataType {
        DATA, ERRORS
    };

    ~Result();

    const std::string errorMessage() const;
    int errorCode() const;

    /**
     * Return the recieved data object, this will be a subclass of Data, i.e. one of @code{String}, @code{Array},
     * @code{Scalar}, etc.
     *
     * @return the data object
     */
    Data* data() const;

    /**
     * Return whether the recieved data has associated error values.
     *
     * If true then @code{errors()} will return the error values.
     *
     * @return true if the data has associated errors
     */
    bool hasErrors() const;

    Data* errors() const;

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

    const std::vector<size_t> shape() const;

    Dim dim(dim_type ndim, DataType data_type) const;

    bool hasTimeDim() const;

    Dim timeDim(DataType data_type) const;

    bool isTree() const
    { return istree_; }

    TreeNode tree() const;

private:
    explicit Result(int handle);

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
    bool istree_;
    std::map<std::string, std::string> meta_;
};

}

#if defined(_WIN32) && !defined(__GNUC__)
#  pragma warning(pop)
#endif

#endif // UDA_WRAPPERS_CPP_RESULT_H

