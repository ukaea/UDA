#include "utils.h"

#include <openssl/bio.h>
#include <openssl/asn1.h>

std::string to_string(const ASN1_TIME* asn1_time)
{
    char work[X509_STRING_SIZE];

    int count = 0;
    BIO* b = BIO_new(BIO_s_mem());
    if (b && ASN1_TIME_print(b, asn1_time)) {
        count = BIO_read(b, work, X509_STRING_SIZE - 1);
        BIO_free(b);
    }
    work[count] = '\0';

    return work;
}