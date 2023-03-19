#ifndef UDA_SOURCE_CLIENT2_PERTHREADSINGLETON_H
#define UDA_SOURCE_CLIENT2_PERTHREADSINGLETON_H

#include <mutex>

#include "client.hpp"

namespace uda {
namespace client {

class ThreadClient
{
public:
    static uda::client::Client& instance();

private:
    ThreadClient() = default;
    ~ThreadClient() = default;

    static uda::client::Client* instance_;
    static std::once_flag init_flag_;

    static void init_client();
};

}
}

#endif //UDA_SOURCE_CLIENT2_PERTHREADSINGLETON_H
