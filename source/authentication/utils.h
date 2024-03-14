#pragma once

#include <string>
#include <openssl/types.h>

#define X509_STRING_SIZE 256

std::string to_string(const ASN1_TIME* time);