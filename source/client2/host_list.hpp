#ifndef UDA_CLIENT_HOSTLIST_H
#define UDA_CLIENT_HOSTLIST_H

#include <vector>
#include <string_view>
#include <string>

#include <clientserver/export.h>

namespace uda {
namespace client {

struct HostData {
    std::string host_alias;
    std::string host_name;
    std::string certificate;
    std::string key;
    std::string ca_certificate;
    int port;
    bool isSSL;
};

class HostList {
public:
    HostList();
    ~HostList() = default;
    const HostData* find_by_alias(std::string_view alias) const;
    const HostData* find_by_name(std::string_view name) const;

private:
    std::vector<HostData> hosts_;
};


}
}

#endif // UDA_CLIENT_HOSTLIST_H
