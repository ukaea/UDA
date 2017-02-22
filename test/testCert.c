#include <ksba.h>

#include <security/security.h>

int testCert2(ksba_cert_t cert)
{
    gpg_error_t err;

    char* dn;
    ksba_isotime_t t;
    int idx;
    ksba_sexp_t sexp;
    const char* oid, * s;

    sexp = ksba_cert_get_serial(cert);
    fputs("  serial....: ", stdout);
    print_sexp(sexp);
    ksba_free(sexp);
    putchar('\n');

    for (idx = 0; (dn = ksba_cert_get_issuer(cert, idx)); idx++) {
        fputs(idx ? "         aka: " : "  issuer....: ", stdout);
        print_dn(dn);
        ksba_free(dn);
        putchar('\n');
    }

    for (idx = 0; (dn = ksba_cert_get_subject(cert, idx)); idx++) {
        fputs(idx ? "         aka: " : "  subject...: ", stdout);
        print_dn(dn);
        ksba_free(dn);
        putchar('\n');
    }

    ksba_cert_get_validity(cert, 0, t);
    fputs("  notBefore.: ", stdout);
    print_time(t);
    putchar('\n');
    ksba_cert_get_validity(cert, 1, t);
    fputs("  notAfter..: ", stdout);
    print_time(t);
    putchar('\n');

    oid = ksba_cert_get_digest_algo(cert);

    /* Under Windows the _ksba_keyinfo_from_sexp are not exported.  */
#ifndef __WIN32
    /* check that the sexp to keyinfo conversion works */
    {
        ksba_sexp_t public;

        public = ksba_cert_get_public_key(cert);
        if (!public) {
            fprintf(stderr, "public key not found\n");
        } else {
            unsigned char* der;
            size_t derlen;

            if (verbose) {
                fputs("  pubkey....: ", stdout);
                print_sexp(public);
                putchar('\n');
            }

            err = _ksba_keyinfo_from_sexp(public, &der, &derlen);
            if (err) {
                fprintf(stderr, "converting public key failed: %s\n", gpg_strerror(err));
            } else {
                ksba_sexp_t tmp;

                fputs("  pubkey-DER: ", stdout);
                print_hex(der, derlen);
                putchar('\n');

                err = _ksba_keyinfo_to_sexp(der, derlen, &tmp);
                if (err) {
                    fprintf(stderr, "re-converting public key failed: %s\n", gpg_strerror(err));
                } else {
                    unsigned char* der2;
                    size_t derlen2;

                    err = _ksba_keyinfo_from_sexp(tmp, &der2, &derlen2);
                    if (err) {
                        fprintf(stderr, "re-re-converting public key failed: %s\n", gpg_strerror(err));
                    } else if (derlen != derlen2 || memcmp(der, der2, derlen)) {
                        fprintf(stderr, "mismatch after re-re-converting public key\n");
                        ksba_free(der2);
                    }
                    ksba_free(tmp);
                }
                ksba_free(der);
            }
        }
    }
#endif

    sexp = ksba_cert_get_sig_val(cert);
    fputs("  sigval....: ", stdout);
    print_sexp(sexp);
    ksba_free(sexp);
    putchar('\n');

    ksba_cert_release(cert);

    return 0;
}

int testCert(const char* fname)
{
    gpg_error_t err;
    FILE* fp;
    ksba_reader_t r;
    ksba_cert_t cert;
    char* dn;
    ksba_isotime_t t;
    int idx;
    ksba_sexp_t sexp;
    const char* oid, * s;

    fp = fopen(fname, "rb");
    if (!fp) {
        fprintf(stderr, "can't open `%s': %s\n", fname, strerror(errno));
        return 999;
    }

    err = ksba_reader_new(&r);
    if (err) {
        fprintf(stderr, "can't create certificate reader\n");
        return 999;
    }

    err = ksba_reader_set_file(r, fp);

    if (err) {
        fprintf(stderr, "can't set file reader\n");
        return 999;
    }

    err = ksba_cert_new(&cert);
    if (err) {
        fprintf(stderr, "can't create new cert object\n");
        return 999;
    }

    err = ksba_cert_read_der(cert, r);
    if (err) {
        fprintf(stderr, "can't read cert\n");
        return 999;
    }

    fprintf(stdout, "Certificate in `%s':\n", fname);

    sexp = ksba_cert_get_serial(cert);
    fputs("  serial....: ", stdout);
    print_sexp(sexp);
    ksba_free(sexp);
    putchar('\n');

    for (idx = 0; (dn = ksba_cert_get_issuer(cert, idx)); idx++) {
        fputs(idx ? "         aka: " : "  issuer....: ", stdout);
        print_dn(dn);
        ksba_free(dn);
        putchar('\n');
    }

    for (idx = 0; (dn = ksba_cert_get_subject(cert, idx)); idx++) {
        fputs(idx ? "         aka: " : "  subject...: ", stdout);
        print_dn(dn);
        ksba_free(dn);
        putchar('\n');
    }

    ksba_cert_get_validity(cert, 0, t);
    fputs("  notBefore.: ", stdout);
    print_time(t);
    putchar('\n');
    ksba_cert_get_validity(cert, 1, t);
    fputs("  notAfter..: ", stdout);
    print_time(t);
    putchar('\n');

    oid = ksba_cert_get_digest_algo(cert);
    //s = (char *)get_oid_desc (oid);
    //printf ("  hash algo.: %s%s%s%s\n", oid, s?" (":"",s?s:"",s?")":"");

    /* Under Windows the _ksba_keyinfo_from_sexp are not exported.  */
#ifndef __WIN32
    /* check that the sexp to keyinfo conversion works */
    {
        ksba_sexp_t public;

        public = ksba_cert_get_public_key(cert);
        if (!public) {
            fprintf(stderr, "public key not found\n");
        } else {
            unsigned char* der;
            size_t derlen;

            if (verbose) {
                fputs("  pubkey....: ", stdout);
                print_sexp(public);
                putchar('\n');
            }

            err = _ksba_keyinfo_from_sexp(public, &der, &derlen);
            if (err) {
                fprintf(stderr, "converting public key failed: %s\n", gpg_strerror(err));
            } else {
                ksba_sexp_t tmp;

                fputs("  pubkey-DER: ", stdout);
                print_hex(der, derlen);
                putchar('\n');

                err = _ksba_keyinfo_to_sexp(der, derlen, &tmp);
                if (err) {
                    fprintf(stderr, "re-converting public key failed: %s\n", gpg_strerror(err));
                } else {
                    unsigned char* der2;
                    size_t derlen2;

                    err = _ksba_keyinfo_from_sexp(tmp, &der2, &derlen2);
                    if (err) {
                        fprintf(stderr, "re-re-converting public key failed: %s\n", gpg_strerror(err));
                    } else if (derlen != derlen2 || memcmp(der, der2, derlen)) {
                        fprintf(stderr, "mismatch after re-re-converting public key\n");
                        ksba_free(der2);
                    }
                    ksba_free(tmp);
                }
                ksba_free(der);
            }
        }
    }
#endif

    sexp = ksba_cert_get_sig_val(cert);
    fputs("  sigval....: ", stdout);
    print_sexp(sexp);
    ksba_free(sexp);
    putchar('\n');

    ksba_cert_release(cert);
    err = ksba_cert_new(&cert);
    if (err) {
        fail_if_err(err);
    }

    err = ksba_cert_read_der(cert, r);
    if (err && gpg_err_code(err) != GPG_ERR_EOF) {
        fprintf(stderr, "expected EOF but got: %s\n", gpg_strerror(err));
    }

    putchar('\n');
    ksba_cert_release(cert);
    ksba_reader_release(r);
    fclose(fp);

    return 0;
}

