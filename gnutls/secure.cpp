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

extern "C" {
#ifndef _MSWINDOWS_
    static int gcrypt_mutex_init(void **mp)
    {
        if(mp)
            *mp = new mutex_t();
        return 0;
    }

    static int gcrypt_mutex_destroy(void **mp)
    {
        if(mp && *mp)
            delete (mutex_t *)(*mp);
        return 0;
    }

    static int gcrypt_mutex_lock(void **mp)
    {
        mutex_t *m = (mutex_t *)(*mp);
        m->acquire();
        return 0;
    }

    static int gcrypt_mutex_unlock(void **mp)
    {
        mutex_t *m = (mutex_t *)(*mp);
        m->release();
        return 0;
    }

    static struct gcry_thread_cbs gcrypt_threading = {
        GCRY_THREAD_OPTION_PTHREAD, NULL,
        gcrypt_mutex_init, gcrypt_mutex_destroy,
        gcrypt_mutex_lock, gcrypt_mutex_unlock
    };
#endif

    static void secure_shutdown(void)
    {
        gnutls_global_deinit();
    }
}

gnutls_priority_t context::priority_cache;

bool secure::fips(void)
{
    return false;
}

bool secure::init(void)
{
    static bool initialized = false;

    if(!initialized) {
        Thread::init();
        Socket::init();

#ifndef _MSWINDOWS_
        gcry_control(GCRYCTL_SET_THREAD_CBS, &gcrypt_threading);
#endif

        gnutls_global_init();
        gnutls_priority_init (&context::priority_cache, "NORMAL", NULL);
        atexit(secure_shutdown);
        initialized = true;
    }
    return true;
}

secure::server_t secure::server(const char *certfile, const char *ca)
{
    context *ctx = new context;

    if(!ctx)
        return NULL;

    ctx->error = secure::OK;
    ctx->connect = GNUTLS_SERVER;
    ctx->xtype = GNUTLS_CRD_CERTIFICATE;
    ctx->xcred = NULL;
    ctx->dh = NULL;
    gnutls_certificate_allocate_credentials(&ctx->xcred);

    gnutls_certificate_set_x509_key_file(ctx->xcred, certfile, certfile, GNUTLS_X509_FMT_PEM);

    if(!ca)
        return ctx;

    if(eq(ca, "*"))
        ca = oscerts();

    gnutls_certificate_set_x509_trust_file (ctx->xcred, ca, GNUTLS_X509_FMT_PEM);

    return ctx;
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

const char *secure::oscerts(void)
{
    const char *path = "c:/temp/ca-bundle.crt";
    if(!is_file(path)) {
        if(oscerts(path))
            return NULL;
    }
    return path;
}

int secure::oscerts(const char *pathname)
{
    bool caset;
    string_t target;

    if(pathname[1] == ':' || pathname[0] == '/' || pathname[0] == '\\')
        target = pathname;
    else
        target = shell::path(shell::USER_CONFIG) + "/" + pathname;

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
        fsys::erase(*target);
        return ENOSYS;
    }
    return 0;
}

#else
const char *secure::oscerts(void)
{
    if(is_file("/etc/ssl/certs/ca-certificates.crt"))
        return "/etc/ssl/certs/ca-certificates.crt";

    if(is_file("/etc/pki/tls/ca-bundle.crt"))
        return "/etc/pki/tls/ca-bundle.crt";

    if(is_file("/etc/ssl/ca-bundle.pem"))
        return "/etc/ssl/ca-bundle.pem";

    return NULL;
}

int secure::oscerts(const char *pathname)
{
    string_t source = oscerts();
    string_t target;

    if(pathname[0] == '/')
        target = pathname;
    else
        target = shell::path(shell::USER_CONFIG) + "/" + pathname;

    if(!source)
        return ENOSYS;

    return fsys::copy(*source, *target);
}
#endif

secure::client_t secure::client(const char *ca)
{
    context *ctx = new context;

    if(!ctx)
        return NULL;

    ctx->error = secure::OK;
    ctx->connect = GNUTLS_CLIENT;
    ctx->xtype = GNUTLS_CRD_CERTIFICATE;
    ctx->xcred = NULL;
    ctx->dh = NULL;
    gnutls_certificate_allocate_credentials(&ctx->xcred);

    if(!ca)
        return ctx;

    if(eq(ca, "*"))
        ca = oscerts();

    gnutls_certificate_set_x509_trust_file (ctx->xcred, ca, GNUTLS_X509_FMT_PEM);

    return ctx;
}

context::~context()
{
    if(dh)
        gnutls_dh_params_deinit(dh);

    if(!xcred)
        return;

    switch(xtype) {
    case GNUTLS_CRD_ANON:
        gnutls_anon_free_client_credentials((gnutls_anon_client_credentials_t)xcred);
        break;
    case GNUTLS_CRD_CERTIFICATE:
        gnutls_certificate_free_credentials(xcred);
        break;
    default:
        break;
    }
}

secure::~secure()
{
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

gnutls_session_t context::session(context *ctx)
{
    SSL ssl = NULL;
    if(ctx && ctx->xcred && ctx->err() == secure::OK) {
        gnutls_init(&ssl, ctx->connect);
        switch(ctx->connect) {
        case GNUTLS_CLIENT:
            gnutls_priority_set_direct(ssl, "PERFORMANCE", NULL);
            break;
        case GNUTLS_SERVER:
            gnutls_priority_set(ssl, context::priority_cache);
            gnutls_certificate_server_set_request(ssl, GNUTLS_CERT_REQUEST);
            gnutls_session_enable_compatibility_mode(ssl);
        default:
            break;
        }
        gnutls_credentials_set(ssl, ctx->xtype, ctx->xcred);
    }
    return ssl;
}


