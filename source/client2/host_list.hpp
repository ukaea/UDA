#ifndef UDA_CLIENT_HOSTLIST_H
#define UDA_CLIENT_HOSTLIST_H

#include <vector>
#include <string_view>
#include <string>

#include <clientserver/export.h>
#include <clientserver/socketStructs.h>

namespace uda {
namespace client {

class HostList {
public:
    HostList();
    ~HostList() = default;
    [[nodiscard]] const HostData* find_by_alias(std::string_view alias) const;
    [[nodiscard]] const HostData* find_by_name(std::string_view name) const;

private:
    std::vector<HostData> hosts_;
};

}
}

#endif // UDA_CLIENT_HOSTLIST_H
