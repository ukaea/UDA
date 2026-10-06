/*
 * Minimal C++ consumer of the installed UDA C++ wrapper.
 *
 * It must compile, link and run using only the flags published by
 * pkg-config for uda-cpp. Including <UDA.hpp> pulls in boost headers, so
 * this fails unless uda-cpp.pc declares where boost lives.
 *
 * No server connection is made, so this is safe to run anywhere.
 */

#include <UDA.hpp>

#include <iostream>

int main()
{
    std::cout << "uda cpp client, default server host: '" << uda::Client::serverHostName() << "'" << std::endl;

    return 0;
}
