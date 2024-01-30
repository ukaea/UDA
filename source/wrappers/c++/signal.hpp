#ifndef UDA_WRAPPERS_CPP_SIGNAL_H
#define UDA_WRAPPERS_CPP_SIGNAL_H

#include "array.hpp"
#include "include/uda/export.h"

#if defined(_WIN32)
#  if !defined(__GNUC__)
#    pragma warning(push)
#    pragma warning(disable: 4251)
#  endif
#endif

namespace uda
{

enum SignalClass {
    ANALYSED, RAW, MODELLED
};

class LIBRARY_API Signal
{
public:
    Signal(const uda::Array& array, SignalClass signal_class, const std::string& alias, const std::string& title,
           long shot, int pass, const std::string& comment="")
        : array_(array)
        , signal_class_(signal_class)
        , alias_(alias)
        , title_(title)
        , shot_(shot)
        , pass_(pass)
        , comment_(comment)
    {

    }

    const uda::Array& array() const { return array_; };
    SignalClass signalClass() const { return signal_class_; }
    const std::string& alias() const { return alias_; }
    const std::string& title() const { return title_; }
    long shot() const { return shot_; }
    int pass() const { return pass_; }
    const std::string& comment() const { return comment_; }
    const std::string& code() const { return code_; }

    void put() const;

private:
    uda::Array array_;
    SignalClass signal_class_;
    std::string alias_;
    std::string title_;
    long shot_;
    int pass_;
    std::string comment_;
    std::string code_;
};

} // namespace

#if defined(_WIN32) && !defined(__GNUC__)
#  pragma warning(pop)
#endif

#endif // UDA_WRAPPERS_CPP_SIGNAL_H
