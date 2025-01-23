#ifndef UDA_CLIENT_HOSTLIST_H
#define UDA_CLIENT_HOSTLIST_H

#include <string>
#include <string_view>
#include <vector>

#include <uda/export.h>
#include "clientserver/socketStructs.h"

namespace uda
{
namespace client
{

class HostList
{
  public:
    HostList();
    ~HostList() = default;
    [[nodiscard]] const uda::client_server::HostData* find_by_alias(std::string_view alias) const;
    [[nodiscard]] const uda::client_server::HostData* find_by_name(std::string_view name) const;

  private:
    std::vector<uda::client_server::HostData> hosts_;
};

} // namespace client
} // namespace uda

#endif // UDA_CLIENT_HOSTLIST_H
