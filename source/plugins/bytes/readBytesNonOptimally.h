#pragma once

#include <string>

#include <clientserver/udaStructs.h>

int readBytes(FILE* file, DATA_BLOCK* data_block, int offset, int max_bytes, const std::string& checksum, bool opaque);

