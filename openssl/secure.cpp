// Copyright (C) 2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include "local.h"

#ifdef  HAVE_OPENSSL_FIPS_H
#include <openssl/fips.h>
#endif

static mutex_t *private_locks = NULL;
static const char *certid = "generic";

extern "C" {
    static void ssl_lock(int mode, int n, const char *file, int line)
    {
        if((mode & 0x03) == CRYPTO_LOCK)
            private_locks[n].acquire();
        else if((mode & 0x03) == CRYPTO_UNLOCK)
            private_locks[n].release();
    }

    static unsigned long ssl_self(void)
    {
#ifdef  _MSWINDOWS_
        return GetCurrentThreadId();
#else
        return (long)Thread::self();
#endif
    }
}

bool secure::fips(const char *progname)
{
#ifdef  HAVE_OPENSSL_FIPS_H

    // must always be first init function called...
    if(private_locks)
        return false;

    if(!FIPS_mode_set(1))
        return false;

    return init(progname);
#else
    return false;
#endif
}

bool secure::init(const char *progname)
{
    if(private_locks)
        return true;

    Thread::init();
    if(progname) {
        certid = progname;
        Socket::init(progname);
    }
    else
        Socket::init();

    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_digests();

    if(CRYPTO_get_id_callback() != NULL)
        return false;

    private_locks = new Mutex[CRYPTO_num_locks()];
    CRYPTO_set_id_callback(ssl_self);
    CRYPTO_set_locking_callback(ssl_lock);
    return true;
}

String secure::path(path_t id)
{
    switch(id) {
    case SYSTEM_CERTIFICATES:
        return str(SSL_CERTS);
    case SYSTEM_KEYS:
        return str(SSL_PRIVATE);
    }
    return str("");
}

#if defined(_MSWINDOWS_)

static void cexport(HCERTSTORE ca, FILE *fp)
{
    PCCERT_CONTEXT cert = NULL;
    const uint8_t *cp;
    char buf[80];

    while ((cert = CertEnumCertificatesInStore(ca, cert)) != NULL) {
        fprintf(fp, "-----BEGIN CERTIFICATE-----\n");
        size_t total = cert->cbCertEncoded;
        size_t count;
        cp = (const uint8_t *)cert->pbCertEncoded;
        while(total) {
            count = String::b64encode(buf, cp, total, 64);
            if(count)
                fprintf(fp, "%s\n", buf);
            total -= count;
            cp += count;
        }
        fprintf(fp, "-----END CERTIFICATE-----\n");
    }
}

int secure::oscerts(const char *pathname)
{
    bool caset;
    string_t target = shell::path(shell::USER_CONFIG) + pathname;
    FILE *fp = fopen(*target, "wt");

    if(!fp)
        return ENOSYS;

    HCERTSTORE ca = CertOpenSystemStoreA((HCRYPTPROV)NULL, "ROOT");
    if(ca) {
        caset = true;
        cexport(ca, fp);
        CertCloseStore(ca, 0);
    }

    ca = CertOpenSystemStoreA((HCRYPTPROV)NULL, "CA");
    if(ca) {
        caset = true;
        cexport(ca, fp);
        CertCloseStore(ca, 0);
    }

    fclose(fp);

    if(!caset) {
        fsys::remove(*target);
        return ENOSYS;
    }
    return 0;
}

#else
int secure::oscerts(const char *pathname)
{
    string_t source = path(SYSTEM_CERTIFICATES) + "/ca-certificates.crt";
    string_t target = shell::path(shell::USER_CONFIG) + pathname;

    if(!*source || !*target)
        return ENOSYS;

    return fsys::copy(*source, *target);
}
#endif

void secure::cipher(secure *scontext, const char *ciphers)
{
    context *ctx = (context *)scontext;
    if(!ctx)
        return;

    SSL_CTX_set_cipher_list(ctx->ctx, ciphers);
}

secure::client_t secure::client(const char *ca)
{
    char certfile[256];

    context *ctx = new(context);
    secure::init();

    if(!ctx)
        return NULL;

    ctx->error = secure::OK;

    ctx->ctx = SSL_CTX_new(SSLv23_client_method());

    if(!ctx->ctx) {
        ctx->error = secure::INVALID;
        return ctx;
    }

    if(!ca)
        return ctx;

    if(eq(ca, "*"))
        ca = SSL_CERTS;
    else if(*ca != '/') {
        snprintf(certfile, sizeof(certfile), "%s/%s.pem", SSL_CERTS, ca);
        ca = certfile;
    }

    if(!SSL_CTX_load_verify_locations(ctx->ctx, ca, 0)) {
        ctx->error = secure::INVALID_AUTHORITY;
        return ctx;
    }

    return ctx;
}

secure::server_t secure::server(const char *ca)
{
    context *ctx = new(context);

    if(!ctx)
        return NULL;

    secure::init();
    ctx->error = secure::OK;
    ctx->ctx = SSL_CTX_new(SSLv23_server_method());

    if(!ctx->ctx) {
        ctx->error = secure::INVALID;
        return ctx;
    }

    char certfile[256];

    snprintf(certfile, sizeof(certfile), "%s/%s.pem", SSL_CERTS, certid);
    if(!SSL_CTX_use_certificate_chain_file(ctx->ctx, certfile)) {
        ctx->error = secure::MISSING_CERTIFICATE;
        return ctx;
    }

    snprintf(certfile, sizeof(certfile), "%s/%s.pem", SSL_PRIVATE, certid);
    if(!SSL_CTX_use_PrivateKey_file(ctx->ctx, certfile, SSL_FILETYPE_PEM)) {
        ctx->error = secure::MISSING_PRIVATEKEY;
        return ctx;
    }

    if(!SSL_CTX_check_private_key(ctx->ctx)) {
        ctx->error = secure::INVALID_CERTIFICATE;
        return ctx;
    }

    if(!ca)
        return ctx;

    if(eq(ca, "*"))
        ca = SSL_CERTS;
    else if(*ca != '/') {
        snprintf(certfile, sizeof(certfile), "%s/%s.pem", SSL_CERTS, ca);
        ca = certfile;
    }

    if(!SSL_CTX_load_verify_locations(ctx->ctx, ca, 0)) {
        ctx->error = secure::INVALID_AUTHORITY;
        return ctx;
    }

    return ctx;
}

secure::error_t secure::verify(session_t session, const char *peername)
{
    SSL *ssl = (SSL *)session;

    char peer_cn[256];

    if(SSL_get_verify_result(ssl) != X509_V_OK)
        return secure::INVALID_CERTIFICATE;

    if(!peername)
        return secure::OK;

    X509 *peer = SSL_get_peer_certificate(ssl);

    if(!peer)
        return secure::INVALID_PEERNAME;

    X509_NAME_get_text_by_NID(
        X509_get_subject_name(peer),
        NID_commonName, peer_cn, sizeof(peer_cn));
    if(!case_eq(peer_cn, peername))
        return secure::INVALID_PEERNAME;

    return secure::OK;
}

secure::~secure()
{
}

context::~context()
{
    if(ctx)
        SSL_CTX_free(ctx);
}

void secure::uuid(char *str)
{
    static unsigned char buf[16];
    static Timer::tick_t prior = 0l;
    static unsigned short seq;
    Timer::tick_t current = Timer::ticks();

    Mutex::protect(&buf);

    // get our (random) node identifier...
    if(!prior)
        Random::fill(buf + 10, 6);

    if(current == prior)
        ++seq;
    else
        Random::fill((unsigned char *)&seq, sizeof(seq));

    buf[8] = (unsigned char)((seq >> 8) & 0xff);
    buf[9] = (unsigned char)(seq & 0xff);
    buf[3] = (unsigned char)(current & 0xff);
    buf[2] = (unsigned char)((current >> 8) & 0xff);
    buf[1] = (unsigned char)((current >> 16) & 0xff);
    buf[0] = (unsigned char)((current >> 24) & 0xff);
    buf[5] = (unsigned char)((current >> 32) & 0xff);
    buf[4] = (unsigned char)((current >> 40) & 0xff);
    buf[7] = (unsigned char)((current >> 48) & 0xff);
    buf[6] = (unsigned char)((current >> 56) & 0xff);

    buf[6] &= 0x0f;
    buf[6] |= 0x10;
    buf[8] |= 0x80;
    String::hexdump(buf, str, "4-2-2-2-6");
    Mutex::release(&buf);
}

String secure::uuid(void)
{
    char buf[38];
    uuid(buf);
    return String(buf);
}


