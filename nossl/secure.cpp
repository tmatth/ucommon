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

#ifdef  _MSWINDOWS_
NAMESPACE_LOCAL
HCRYPTPROV _handle = (HCRYPTPROV)NULL;
END_NAMESPACE
#endif

secure::~secure()
{
}

bool secure::fips(const char *progname)
{
    return false;
}

bool secure::init(const char *progname)
{
    Thread::init();
    if(progname)
        Socket::init(progname);
    else
        Socket::init();

#ifdef  _MSWINDOWS_
    if(_handle != (HCRYPTPROV)NULL)
        return false;

    if(CryptAcquireContext(&_handle, NULL, NULL, PROV_RSA_FULL, 0))
        return false;
    if(GetLastError() == (DWORD)NTE_BAD_KEYSET) {
        if(CryptAcquireContext(&_handle, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
            return false;
    }

    _handle = (HCRYPTPROV)NULL;
#endif

    return false;
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

secure::server_t secure::server(const char *ca)
{
    return NULL;
}

secure::client_t secure::client(const char *ca)
{
    return NULL;
}

void secure::cipher(secure *context, const char *ciphers)
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

