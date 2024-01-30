#include "x509Utils.h"

#include <cerrno>

#include "clientserver/errorLog.h"
#include "clientserver/stringUtils.h"

#include "security.h"

#define digitp(p) (*(p) >= '0' && *(p) <= '9')
#define xfree(a) ksba_free(a)
#define xtrymalloc(a) gcry_malloc((a))

#define HASH_FNC ((void (*)(void*, const void*, size_t))gcry_md_write)
#define DIM(v) (sizeof(v) / sizeof((v)[0]))

#define hexdigitp(a) (digitp(a) || (*(a) >= 'A' && *(a) <= 'F') || (*(a) >= 'a' && *(a) <= 'f'))
#define xtoi_1(p) (*(p) <= '9' ? (*(p) - '0') : *(p) <= 'F' ? (*(p) - 'A' + 10) : (*(p) - 'a' + 10))
#define xtoi_2(p) ((xtoi_1(p) * 16) + xtoi_1((p) + 1))

//========================================================================================================
// Components taken from

/* fipsdrv.c  -  A driver to help with FIPS CAVS tests.
   Copyright (C) 2008 Free Software Foundation, Inc.

   This file is part of Libgcrypt.

   Libgcrypt is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   Libgcrypt is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* ASN.1 classes.  */
enum { UNIVERSAL = 0, APPLICATION = 1, ASNCONTEXT = 2, PRIVATE = 3 };

/* ASN.1 tags.  */
enum {
    TAG_NONE = 0,
    TAG_BOOLEAN = 1,
    TAG_INTEGER = 2,
    TAG_BIT_STRING = 3,
    TAG_OCTET_STRING = 4,
    TAG_nullptr = 5,
    TAG_OBJECT_ID = 6,
    TAG_OBJECT_DESCRIPTOR = 7,
    TAG_EXTERNAL = 8,
    TAG_REAL = 9,
    TAG_ENUMERATED = 10,
    TAG_EMBEDDED_PDV = 11,
    TAG_UTF8_STRING = 12,
    TAG_REALTIVE_OID = 13,
    TAG_SEQUENCE = 16,
    TAG_SET = 17,
    TAG_NUMERIC_STRING = 18,
    TAG_PRINTABLE_STRING = 19,
    TAG_TELETEX_STRING = 20,
    TAG_VIDEOTEX_STRING = 21,
    TAG_IA5_STRING = 22,
    TAG_UTC_TIME = 23,
    TAG_GENERALIZED_TIME = 24,
    TAG_GRAPHIC_STRING = 25,
    TAG_VISIBLE_STRING = 26,
    TAG_GENERAL_STRING = 27,
    TAG_UNIVERSAL_STRING = 28,
    TAG_CHARACTER_STRING = 29,
    TAG_BMP_STRING = 30
};

/* ASN.1 Parser object.  */
struct tag_info {
    int class;             /* Object class.  */
    unsigned long tag;     /* The tag of the object.  */
    unsigned long length;  /* Length of the values.  */
    int nhdr;              /* Length of the header (TL).  */
    unsigned int ndef : 1; /* The object has an indefinite length.  */
    unsigned int cons : 1; /* This is a constructed object.  */
};

/* Return the number of bits of the Q parameter from the DSA key
   KEY.  */
static unsigned int get_dsa_qbits(gcry_sexp_t key)
{
    gcry_sexp_t l1, l2;
    gcry_mpi_t q;
    unsigned int nbits;

    l1 = gcry_sexp_find_token(key, "public-key", 0);
    if (!l1) {
        return 0;
    } /* Does not contain a key object.  */
    l2 = gcry_sexp_cadr(l1);
    gcry_sexp_release(l1);
    l1 = gcry_sexp_find_token(l2, "q", 1);
    gcry_sexp_release(l2);
    if (!l1) {
        return 0;
    } /* Invalid object.  */
    q = gcry_sexp_nth_mpi(l1, 1, GCRYMPI_FMT_USG);
    gcry_sexp_release(l1);
    if (!q) {
        return 0;
    } /* Missing value.  */
    nbits = gcry_mpi_get_nbits(q);
    gcry_mpi_release(q);

    return nbits;
}

static int do_encode_md(gcry_md_hd_t md, int algo, int pkalgo, unsigned int nbits, gcry_sexp_t pkey, gcry_mpi_t* r_val)
{
    int n;
    size_t nframe;
    unsigned char* frame;

    if (pkalgo == GCRY_PK_DSA || pkalgo == GCRY_PK_ECDSA) {
        unsigned int qbits;

        if (pkalgo == GCRY_PK_ECDSA) {
            qbits = gcry_pk_get_nbits(pkey);
        } else {
            qbits = get_dsa_qbits(pkey);
        }

        if ((qbits % 8)) {
            return 999;
        }

        /* Don't allow any Q smaller than 160 bits.  We don't want
       someone to issue signatures from a key with a 16-bit Q or
       something like that, which would look correct but allow
       trivial forgeries.  Yes, I know this rules out using MD5 with
       DSA. ;) */
        if (qbits < 160) {
            return 999;
        }

        /* Check if we're too short.  Too long is safe as we'll
       automatically left-truncate. */
        nframe = gcry_md_get_algo_dlen(algo);
        if (nframe < qbits / 8) {
            return 999;
            /* FIXME: we need to check the requirements for ECDSA.  */
            if (nframe < 20 || pkalgo == GCRY_PK_DSA) {
                return 999;
            }
        }

        frame = (unsigned char*)xtrymalloc(nframe);
        if (!frame) {
            return 999;
        }

        memcpy(frame, gcry_md_read(md, algo), nframe);
        n = nframe;
        /* Truncate.  */
        if (n > qbits / 8) {
            n = qbits / 8;
        }
    } else {
        unsigned char asn[100];
        size_t asnlen;
        size_t len;

        nframe = (nbits + 7) / 8;

        asnlen = DIM(asn);
        if (!algo || gcry_md_test_algo(algo)) {
            return 999;
        }
        if (gcry_md_algo_info(algo, GCRYCTL_GET_ASNOID, asn, &asnlen)) {
            return 999;
        }

        len = gcry_md_get_algo_dlen(algo);

        if (len + asnlen + 4 > nframe) {
            return 999;
        }

        /* We encode the MD in this way:
         *
         *       0  A PAD(n bytes)   0  ASN(asnlen bytes)  MD(len bytes)
         *
         * PAD consists of FF bytes.
         */
        frame = (unsigned char*)xtrymalloc(nframe);
        if (!frame) {
            return 999;
        }

        n = 0;
        frame[n++] = 0;
        frame[n++] = 1; /* block type */
        i = nframe - len - asnlen - 3;

        if (!((i > 1))) {
            return 999;
        }
        memset(frame + n, 0xff, i);
        n += i;
        frame[n++] = 0;
        memcpy(frame + n, asn, asnlen);
        n += asnlen;
        memcpy(frame + n, gcry_md_read(md, algo), len);
        n += len;

        if (!((n == nframe))) {
            return 999;
        }
    }

    gcry_mpi_scan(r_val, GCRYMPI_FMT_USG, frame, n, &nframe);
    xfree(frame);
    return 0;
}

/**
 * Return the public key algorithm id from the S-expression PKEY.
 *
 * FIXME: libgcrypt should provide such a function. Note that this implementation uses the names as used by libksba.
 * @param pkey
 * @return
 */
static int pk_algo_from_sexp(gcry_sexp_t pkey)
{
    gcry_sexp_t l1, l2;
    const char* name;
    size_t n;
    int algo;

    l1 = gcry_sexp_find_token(pkey, "public-key", 0);
    if (!l1) {
        return 0;
    } /* Not found.  */
    l2 = gcry_sexp_cadr(l1);
    gcry_sexp_release(l1);

    name = gcry_sexp_nth_data(l2, 0, &n);
    if (!name) {
        algo = 0; /* Not found. */
    } else if (n == 3 && !memcmp(name, "rsa", 3)) {
        algo = GCRY_PK_RSA;
    } else if (n == 3 && !memcmp(name, "dsa", 3)) {
        algo = GCRY_PK_DSA;
        /* Because this function is called only for verification we can
           assume that ECC actually means ECDSA.  */
    } else if (n == 3 && !memcmp(name, "ecc", 3)) {
        algo = GCRY_PK_ECDSA;
    } else if (n == 13 && !memcmp(name, "ambiguous-rsa", 13)) {
        algo = GCRY_PK_RSA;
    } else {
        algo = 0;
    }
    gcry_sexp_release(l2);
    return algo;
}

/**
 * Read a file from stream FP into a newly allocated buffer and return that buffer.
 * The valid length of the buffer is stored at R_LENGTH. Returns nullptr on failure.  If decode is set, the file is
 * assumed to be hex encoded and the decoded content is returned.
 * @param fp
 * @param decode
 * @param r_length
 * @return
 */
static void* read_file(FILE* fp, int decode, size_t* r_length)
{
    char* buffer;
    size_t buflen;
    size_t nread, bufsize = 0;

    *r_length = 0;
#define NCHUNK 8192
#ifdef HAVE_DOSISH_SYSTEM
    setmode(fileno(fp), O_BINARY);
#endif
    buffer = nullptr;
    buflen = 0;
    do {
        bufsize += NCHUNK;
        if (!buffer) {
            buffer = gcry_xmalloc(bufsize);
        } else {
            buffer = gcry_xrealloc(buffer, bufsize);
        }

        nread = fread(buffer + buflen, 1, NCHUNK, fp);
        if (nread < NCHUNK && ferror(fp)) {
            gcry_free(buffer);
            return nullptr;
        }
        buflen += nread;
    } while (nread == NCHUNK);
#undef NCHUNK
    if (decode) {
        const char* s;
        char* p;

        for (s = buffer, p = buffer, nread = 0; nread + 1 < buflen; s += 2, nread += 2) {
            if (!hexdigitp(s) || !hexdigitp(s + 1)) {
                gcry_free(buffer);
                return nullptr; /* Invalid hex digits. */
            }
            *(unsigned char*)p++ = xtoi_2(s);
        }
        if (nread != buflen) {
            gcry_free(buffer);
            return nullptr; /* Odd number of hex digits. */
        }
        buflen = p - buffer;
    }

    *r_length = buflen;
    return buffer;
}

/**
 * Do in-place decoding of base-64 data of LENGTH in BUFFER.
 * @param buffer
 * @param length
 * @param newLength
 * @return the new length of the buffer.
 */
static int base64_decode(char* buffer, size_t length, size_t* newLength)
{
    static unsigned char const asctobin[128] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
        0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff};

    int err = 0;
    int idx = 0;
    unsigned char val = 0;
    int c = 0;
    char *d, *s;
    int lfseen = 1;

    *newLength = 0;

    /* Find BEGIN line.  */
    for (s = buffer; length; length--, s++) {
        if (lfseen && *s == '-' && length > 11 && !memcmp(s, "-----BEGIN ", 11)) {
            for (; length && *s != '\n'; length--, s++)
                ;
            break;
        }
        lfseen = (*s == '\n');
    }

    /* Decode until pad character or END line.  */
    for (d = buffer; length; length--, s++) {
        if (lfseen && *s == '-' && length > 9 && !memcmp(s, "-----END ", 9)) {
            break;
        }
        if ((lfseen = (*s == '\n')) || *s == ' ' || *s == '\r' || *s == '\t') {
            continue;
        }
        if (*s == '=') {
            /* Pad character: stop */
            if (idx == 1) {
                *d++ = val;
            }
            break;
        }

        if ((*s & 0x80) || (c = asctobin[*(unsigned char*)s]) == 0xff) {
            err = 999;
            // die ("invalid base64 character %02X at pos %d detected\n", *(unsigned char*)s, (int)(s-buffer));
            return err;
        }

        switch (idx) {
            case 0:
                val = c << 2;
                break;
            case 1:
                val |= (c >> 4) & 3;
                *d++ = val;
                val = (c << 4) & 0xf0;
                break;
            case 2:
                val |= (c >> 2) & 15;
                *d++ = val;
                val = (c << 6) & 0xc0;
                break;
            case 3:
                val |= c & 0x3f;
                *d++ = val;
                break;
        }
        idx = (idx + 1) % 4;
    }

    *newLength = d - buffer;
    return err;
}

/**
 * Parse the buffer at the address BUFFER which consists of the number of octets as stored at BUFLEN.
 * Return the tag and the length part from the TLV triplet. Update BUFFER and BUFLEN on success. Checks that the
 * encoded length does not exhaust the length of the provided buffer.
 * @param buffer
 * @param buflen
 * @param ti
 * @return
 */
static int parse_tag(unsigned char const** buffer, size_t* buflen, struct tag_info* ti)
{
    int c;
    unsigned long tag;
    const unsigned char* buf = *buffer;
    size_t length = *buflen;

    ti->length = 0;
    ti->ndef = 0;
    ti->nhdr = 0;

    /* Get the tag */
    if (!length) {
        return -1;
    } /* Premature EOF.  */
    c = *buf++;
    length--;
    ti->nhdr++;

    ti->class = (c & 0xc0) >> 6;
    ti->cons = !!(c & 0x20);
    tag = (c & 0x1f);

    if (tag == 0x1f) {
        tag = 0;
        do {
            tag <<= 7;
            if (!length) {
                return -1;
            } /* Premature EOF.  */
            c = *buf++;
            length--;
            ti->nhdr++;
            tag |= (c & 0x7f);
        } while ((c & 0x80));
    }
    ti->tag = tag;

    /* Get the length */
    if (!length) {
        return -1;
    } /* Premature EOF. */
    c = *buf++;
    length--;
    ti->nhdr++;

    if (!(c & 0x80)) {
        ti->length = c;
    } else if (c == 0x80) {
        ti->ndef = 1;
    } else if (c == 0xff) {
        return -1; /* Forbidden length value.  */
    } else {
        unsigned long len = 0;
        int count = c & 0x7f;

        for (; count; count--) {
            len <<= 8;
            if (!length) {
                return -1; /* Premature EOF.  */
            }
            c = *buf++;
            length--;
            ti->nhdr++;
            len |= (c & 0xff);
        }
        ti->length = len;
    }

    if (ti->class == UNIVERSAL && !ti->tag) {
        ti->length = 0;
    }

    if (ti->length > length) {
        return -1; /* Data larger than buffer.  */
    }

    *buffer = buf;
    *buflen = length;
    return 0;
}

/**
 * Import a Security Document from a private file (a key or X.509 certificate)
 * @param file
 * @param contents
 * @param length
 * @return
 */
int importSecurityDoc(const char* file, unsigned char** contents, unsigned short* length)
{
    int err = 0;

    *length = 0;
    *contents = (unsigned char*)malloc(UDA_MAXKEY * sizeof(unsigned char));

    FILE* fd = nullptr;

    errno = 0;

    if (((fd = fopen(file, "rb")) == nullptr || ferror(fd) || errno != 0)) {
        if (fd != nullptr) {
            fclose(fd);
        }
        UDA_THROW_ERROR(999, "Cannot open the security document: key or certificate");
    }

    size_t fileLength = fread(*contents, sizeof(char), UDA_MAXKEY, fd);

    if (!feof(fd) || fileLength == UDA_MAXKEY) {
        free(*contents);
        fclose(fd);
        UDA_THROW_ERROR(999, "Security document length limit hit!");
    }

    *length = (unsigned short)fileLength;

    fclose(fd);
    return err;
}

int makeX509CertObject(unsigned char* doc, unsigned short docLength, ksba_cert_t* cert)
{
    int err = 0;
    ksba_cert_t certificate;

    if (ksba_cert_new(&certificate) != 0) {
        UDA_THROW_ERROR(999, "Problem creating the certificate object!");
    }

    if (ksba_cert_init_from_mem(certificate, (const void*)doc, (size_t)docLength) != 0) {
        UDA_THROW_ERROR(999, "Problem initialising the certificate object!");
    }

    *cert = certificate;

    return err;
}

/**
 * Extract a Public Key from a X509 certificate file and return an S-Expression
 * @param cert
 * @param key_sexp
 * @return
 */
int extractX509SExpKey(ksba_cert_t cert, gcry_sexp_t* key_sexp)
{
    int err = 0;

    ksba_sexp_t p;
    size_t n;

    if ((p = ksba_cert_get_public_key(cert)) == nullptr) {
        UDA_THROW_ERROR(999, "Failure to get the Public key!");
    }

    // Get the length of the canonical S-Expression (public key)

    if ((n = gcry_sexp_canon_len(p, 0, nullptr, nullptr)) == 0) {
        ksba_free(p);
        UDA_THROW_ERROR(999, "did not return a proper S-Exp!");
    }

    // Create an internal S-Expression from the external representation

    if (gcry_sexp_sscan(key_sexp, nullptr, (char*)p, n) != 0) {
        ksba_free(p);
        UDA_THROW_ERROR(999, "S-Exp creation failed!");
    }

    ksba_free(p);

    return err;
}

int importX509Reader(const char* fileName, ksba_cert_t* cert)
{
    int err = 0;
    FILE* fp;
    ksba_reader_t r;

    errno = 0;

    fp = fopen(fileName, "rb");
    if (!fp) {
        addIdamError(UDA_CODE_ERROR_TYPE, __func__, 999, "Problem opening the certificate file");
        UDA_THROW_ERROR(999, strerror(errno));
    }

    err = ksba_reader_new(&r);
    if (err) {
        UDA_THROW_ERROR(999, "can't create certificate reader");
    }

    err = ksba_reader_set_file(r, fp);
    if (err) {
        UDA_THROW_ERROR(999, "can't set file reader");
    }

    err = ksba_cert_new(cert);
    if (err) {
        UDA_THROW_ERROR(999, "Problem creating a new certificate object!");
    }

    err = ksba_cert_read_der(*cert, r);
    if (err) {
        UDA_THROW_ERROR(999, "Problem initialising the certificate object!");
    }

    fclose(fp);

    return err;
}

/**
 * Test the X509 certificate dates are valid
 * @param certificate
 * @return
 */
int testX509Dates(ksba_cert_t certificate)
{
    int err = 0;
    ksba_isotime_t startDateTime = {};
    ksba_isotime_t endDateTime = {}; // ISO format referenced from UTC

    ksba_cert_get_validity(certificate, 0, startDateTime);
    ksba_cert_get_validity(certificate, 1, endDateTime);

    // Current Date and Time

    time_t calendar;                  // Simple Calendar Date & Time
    struct tm* broken;                // Broken Down calendar Time
    static char datetime[DATELENGTH]; // The Calendar Time as a formatted String

    // Calendar Time

    time(&calendar);
    broken = gmtime(&calendar);
#ifndef _WIN32
    asctime_r(broken, datetime);
#else
    asctime_s(datetime, DATELENGTH, broken);
#endif

    convertNonPrintable2(datetime);
    TrimString(datetime);

    // Year

    char work[56];
    sprintf(work, "%.4d%.2d%.2dT%.2d%.2d%.2d", broken->tm_year + 1900, broken->tm_mon + 1, broken->tm_mday,
            broken->tm_hour, broken->tm_min, broken->tm_sec);

    if ((strcmp(work, startDateTime) <= 0) || (strcmp(endDateTime, work) <= 0)) { // dates are in ascending order
        UDA_THROW_ERROR(999, "X509 Certificate is Invalid: Time Expired!");
    }

    return err;
}

/**
 * Check the certificate signature.
 * based on GNUPG sm/certcheck.c (gpgsm_check_cert_sig)
 * @param issuer_cert
 * @param cert
 * @return
 */
int checkX509Signature(ksba_cert_t issuer_cert, ksba_cert_t cert)
{
    int err = 0;

    const char* algoid = nullptr;

    // Extract the digest algorithm OID used for the signature

    if ((algoid = ksba_cert_get_digest_algo(cert)) == nullptr) {
        UDA_THROW_ERROR(999, "unknown digest algorithm OID");
    }

    // Map the algorithm OID to an algorithm identifier

    int algo;
    if ((algo = gcry_md_map_name(algoid)) == 0) {
        UDA_THROW_ERROR(999, "unknown digest algorithm identifier");
    }

    // Create a new digest object with the same algorithm as the certificate signature

    gcry_md_hd_t md;
    if (gcry_md_open(&md, algo, 0) != 0) {
        UDA_THROW_ERROR(999, "md_open failed!");
    }

    // Hash the certificate

    if (ksba_cert_hash(cert, 1, HASH_FNC, md) != 0) {
        gcry_md_close(md);
        UDA_THROW_ERROR(999, "cert hash failed!");
    }

    // Finalise the digest calculation

    gcry_md_final(md);

    // Get the certificate signature

    ksba_sexp_t p;
    if ((p = ksba_cert_get_sig_val(cert)) == nullptr) {
        gcry_md_close(md);
        UDA_THROW_ERROR(999, "Failure to get the certificate signature!");
    }

    // Get the length of the canonical S-Expression (certificate signature)

    size_t n;
    if ((n = gcry_sexp_canon_len(p, 0, nullptr, nullptr)) == 0) {
        gcry_md_close(md);
        ksba_free(p);
        UDA_THROW_ERROR(999, "libksba did not return a proper S-Exp!");
    }

    // Create an internal S-Expression from the external representation

    gcry_sexp_t s_sig;
    if (gcry_sexp_sscan(&s_sig, nullptr, (char*)p, n) != 0) {
        gcry_md_close(md);
        UDA_THROW_ERROR(999, "gcry_sexp_scan failed!");
    }

    ksba_free(p);

    // Get the CA Public key

    if ((p = ksba_cert_get_public_key(issuer_cert)) == nullptr) {
        gcry_md_close(md);
        UDA_THROW_ERROR(999, "Failure to get the Public key!");
    }

    // Get the length of the canonical S-Expression (public key)

    if ((n = gcry_sexp_canon_len(p, 0, nullptr, nullptr)) == 0) {
        gcry_md_close(md);
        ksba_free(p);
        gcry_sexp_release(s_sig);
        UDA_THROW_ERROR(999, "libksba did not return a proper S-Exp!");
    }

    // Create an internal S-Expression from the external representation

    gcry_sexp_t s_pkey;
    if (gcry_sexp_sscan(&s_pkey, nullptr, (char*)p, n) != 0) {
        gcry_md_close(md);
        ksba_free(p);
        gcry_sexp_release(s_sig);
        UDA_THROW_ERROR(999, "gcry_sexp_scan failed!");
    }

    ksba_free(p);

    gcry_mpi_t frame;

    if (do_encode_md(md, algo, pk_algo_from_sexp(s_pkey), gcry_pk_get_nbits(s_pkey), s_pkey, &frame) != 0) {
        gcry_md_close(md);
        gcry_sexp_release(s_sig);
        gcry_sexp_release(s_pkey);
        UDA_THROW_ERROR(999, "do_encode_md failed!");
    }

    // put hash into the S-Exp s_hash

    gcry_sexp_t s_hash;
    gcry_sexp_build(&s_hash, nullptr, "%m", frame);

    gcry_mpi_release(frame);

    // Verify the signature, data, public key

    if (gcry_pk_verify(s_sig, s_hash, s_pkey) != 0) {
        UDA_THROW_ERROR(999, "Signature verification failed!");
    }

    gcry_md_close(md);
    gcry_sexp_release(s_sig);
    gcry_sexp_release(s_hash);
    gcry_sexp_release(s_pkey);

    return err;
}

/**
 * Import a Private Key from a PEM file and return an S-Expression.
 * based on fipsdrv.c#read_private_key_file
 * @param keyFile
 * @param key_sexp
 * @return
 */
int importPEMPrivateKey(const char* keyFile, gcry_sexp_t* key_sexp)
{
    int err = 0;

    gcry_mpi_t keyparms[8];
    int n_keyparms = 8;
    gcry_sexp_t s_key;

    FILE* fp;
    if ((fp = fopen(keyFile, "rb")) == nullptr) {
        UDA_THROW_ERROR(999, "Failed to open private key file");
    }

    size_t buflen;
    char* buffer = read_file(fp, 0, &buflen);
    fclose(fp);

    if (!buffer) {
        UDA_THROW_ERROR(999, "Failed to read private key file");
    }

    size_t oldBuflen = buflen;
    if ((err = base64_decode(buffer, oldBuflen, &buflen)) != 0) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Failed to decode private key buffer");
    }

    // Parse the ASN.1 structure.

    const unsigned char* der = (const unsigned char*)buffer;
    size_t derlen = buflen;

    struct tag_info ti;

    if (parse_tag(&der, &derlen, &ti) || ti.tag != TAG_SEQUENCE || ti.class || !ti.cons || ti.ndef) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Failed to parse tag from private key buffer");
    }

    if (parse_tag(&der, &derlen, &ti) || ti.tag != TAG_INTEGER || ti.class || ti.cons || ti.ndef) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Failed to parse tag from private key buffer");
    }

    if (ti.length != 1 || *der) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Incorrect tag length");
    }

    der += ti.length;
    derlen -= ti.length;

    int idx;
    for (idx = 0; idx < n_keyparms; idx++) {

        if (parse_tag(&der, &derlen, &ti) || ti.tag != TAG_INTEGER || ti.class || ti.cons || ti.ndef) {
            gcry_free(buffer);
            UDA_THROW_ERROR(999, "Failed to parse tag from private key buffer");
        }

        gcry_error_t gerr = gcry_mpi_scan(keyparms + idx, GCRYMPI_FMT_USG, der, ti.length, nullptr);

        if (gerr) {
            gcry_free(buffer);
            UDA_THROW_ERROR(999, gcry_strerror(gerr));
        }

        der += ti.length;
        derlen -= ti.length;
    }

    if (idx != n_keyparms) {
        gcry_free(buffer);
        UDA_THROW_ERROR(999, "Incorrect number of key params in private key");
    }

    gcry_free(buffer);

    // Convert from OpenSSL parameter ordering to the OpenPGP order.
    // First check that p < q; if not swap p and q and recompute u.

    if (gcry_mpi_cmp(keyparms[3], keyparms[4]) > 0) {
        gcry_mpi_swap(keyparms[3], keyparms[4]);
        gcry_mpi_invm(keyparms[7], keyparms[3], keyparms[4]);
    }

    // Build the S-expression.

    gcry_error_t gerr = gcry_sexp_build(&s_key, nullptr, "(private-key(rsa(n%m)(e%m)(d%m)(p%m)(q%m)(u%m)))",
                                        keyparms[0], keyparms[1], keyparms[2], keyparms[3], keyparms[4], keyparms[7]);

    if (gerr) {
        gcry_free(buffer);
        UDA_THROW_ERROR(999, gcry_strerror(gerr));
    }

    for (idx = 0; idx < n_keyparms; idx++) {
        gcry_mpi_release(keyparms[idx]);
    }

    *key_sexp = s_key;

    return err;
}

/**
 * Import a Public Key from a PEM file and return an S-Expression.
 * based on fipsdrv.c#read_public_key_file
 * @param keyFile
 * @param key_sexp
 * @return
 */
int importPEMPublicKey(char* keyFile, gcry_sexp_t* key_sexp)
{
    int err = 0;

    FILE* fp;
    if ((fp = fopen(keyFile, "rb")) == nullptr) {
        UDA_THROW_ERROR(999, "Failed to open public key file");
    }

    size_t buflen;
    char* buffer = read_file(fp, 0, &buflen);
    fclose(fp);

    if (!buffer) {
        UDA_THROW_ERROR(999, "Failed to read public key file");
    }

    size_t oldBuflen = buflen;
    if ((err = base64_decode(buffer, oldBuflen, &buflen)) != 0) {
        gcry_free(buffer);
        UDA_THROW_ERROR(999, "Failed to decode public key");
    }

    // Parse the ASN.1 structure.

    const unsigned char* der = (const unsigned char*)buffer;
    size_t derlen = buflen;
    struct tag_info ti;

    if (parse_tag(&der, &derlen, &ti) || ti.tag != TAG_SEQUENCE || ti.class || !ti.cons || ti.ndef) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Failed to parse tag from public key buffer");
    }

    if (parse_tag(&der, &derlen, &ti) || ti.tag != TAG_SEQUENCE || ti.class || !ti.cons || ti.ndef) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Failed to parse tag from public key buffer");
    }

    // We skip the description of the key parameters and assume it is RSA.

    der += ti.length;
    derlen -= ti.length;

    if (parse_tag(&der, &derlen, &ti) || ti.tag != TAG_BIT_STRING || ti.class || ti.cons || ti.ndef) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Failed to parse tag from public key buffer");
    }

    if (ti.length < 1 || *der) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Incorrect tag length");
    }

    der += 1;
    derlen -= 1;

    // Parse the BIT string.

    if (parse_tag(&der, &derlen, &ti) || ti.tag != TAG_SEQUENCE || ti.class || !ti.cons || ti.ndef) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Failed to parse tag from public key buffer");
    }

    int idx;
    int n_keyparms = 2;
    gcry_mpi_t keyparms[2];

    for (idx = 0; idx < n_keyparms; idx++) {

        if (parse_tag(&der, &derlen, &ti) || ti.tag != TAG_INTEGER || ti.class || ti.cons || ti.ndef) {
            gcry_free(buffer);
            UDA_THROW_ERROR(err, "Failed to parse tag from public key buffer");
        }

        gcry_error_t gerr = gcry_mpi_scan(keyparms + idx, GCRYMPI_FMT_USG, der, ti.length, nullptr);

        if (gerr) {
            gcry_free(buffer);
            UDA_THROW_ERROR(err, gcry_strerror(gerr));
        }
        der += ti.length;
        derlen -= ti.length;
    }

    if (idx != n_keyparms) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, "Incorrect number of params in public key");
    }

    gcry_free(buffer);

    // Build the S-expression.

    gcry_sexp_t s_key;

    gcry_error_t gerr = gcry_sexp_build(&s_key, nullptr, "(public-key(rsa(n%m)(e%m)))", keyparms[0], keyparms[1]);
    if (gerr) {
        gcry_free(buffer);
        UDA_THROW_ERROR(err, gcry_strerror(gerr));
    }

    for (idx = 0; idx < n_keyparms; idx++) {
        gcry_mpi_release(keyparms[idx]);
    }

    *key_sexp = s_key;

    return err;
}