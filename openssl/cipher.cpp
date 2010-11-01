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

Cipher::Key::Key(const char *cipher, const char *digest, const char *text, size_t size, const unsigned char *salt, unsigned rounds)
{
    secure::init();

    clear();

    // never use sha0...
    if(ieq(digest, "sha"))
        digest = "sha1";

    char algoname[64];
    String::set(algoname, sizeof(algoname), cipher);
    char *fpart = strchr(algoname, '-');
    char *lpart = strrchr(algoname, '-');

    if(fpart && fpart == lpart)
        strcpy(fpart, fpart + 1);

    algotype = EVP_get_cipherbyname(algoname);
    hashtype = EVP_get_digestbyname(digest);

    if(!algotype || !hashtype)
        return;

    if(!size)
        size = strlen((const char *)text);

    keysize = EVP_BytesToKey((const EVP_CIPHER*)algotype, (const EVP_MD*)hashtype, salt, (const unsigned char *)text, size, rounds, keybuf, ivbuf);
    if(keysize)
        blksize = EVP_CIPHER_block_size((const EVP_CIPHER*)algotype);
}

Cipher::Key::Key()
{
    secure::init();
    clear();
}

Cipher::Key::~Key()
{
    clear();
}

void Cipher::Key::clear(void)
{
    algotype = NULL;
    hashtype = NULL;
    keysize = blksize = 0;

    zerofill(keybuf, sizeof(keybuf));
    zerofill(ivbuf, sizeof(ivbuf));
}

Cipher::Cipher(key_t key, mode_t mode, unsigned char *address, size_t size)
{
    bufaddr = NULL;
    bufsize = bufpos = 0;
    context = NULL;
    set(key, mode, address, size);
}

Cipher::Cipher()
{
    bufaddr = NULL;
    bufsize = bufpos = 0;
    context = NULL;
}

Cipher::~Cipher()
{
    flush();
    release();
}

bool Cipher::is(const char *id)
{
    // make sure cipher-bitsize forms without -mode do not fail...
    char algoname[64];
    String::set(algoname, sizeof(algoname), id);
    char *fpart = strchr(algoname, '-');
    char *lpart = strrchr(algoname, '-');

    if(fpart && fpart == lpart)
        strcpy(fpart, fpart + 1);

    return (EVP_get_cipherbyname(algoname) != NULL);
}

void Cipher::push(unsigned char *address, size_t size)
{
}

void Cipher::release(void)
{
    keys.clear();
    if(context) {
        EVP_CIPHER_CTX_cleanup((EVP_CIPHER_CTX*)context);
        delete (EVP_CIPHER_CTX*)context;
        context = NULL;
    }
}

size_t Cipher::flush(void)
{
    size_t total = bufpos;

    if(bufpos && bufsize) {
        push(bufaddr, bufpos);
        bufpos = 0;
    }
    bufaddr = NULL;
    return total;
}

size_t Cipher::puts(const char *text)
{
    char padbuf[64];
    if(!text || !bufaddr)
        return 0;

    size_t len = strlen(text) + 1;
    unsigned pad = len % keys.iosize();

    size_t count = put((const unsigned char *)text, len - pad);
    if(pad) {
        memcpy(padbuf, text + len - pad, pad);
        memset(padbuf + pad, 0, keys.iosize() - pad);
        count += put((const unsigned char *)padbuf, keys.iosize());
        zerofill(padbuf, sizeof(padbuf));
    }
    return flush();
}

void Cipher::set(unsigned char *address, size_t size)
{
    flush();
    bufsize = size;
    bufaddr = address;
    bufpos = 0;
}

void Cipher::set(key_t key, mode_t mode, unsigned char *address, size_t size)
{
    release();

    bufsize = size;
    bufmode = mode;
    bufaddr = address;

    memcpy(&keys, key, sizeof(keys));
    if(!keys.keysize)
        return;

    context = new EVP_CIPHER_CTX;
    EVP_CIPHER_CTX_init((EVP_CIPHER_CTX *)context);
    EVP_CipherInit_ex((EVP_CIPHER_CTX *)context, (EVP_CIPHER *)keys.algotype, NULL, keys.keybuf, keys.ivbuf, (int)mode);
    EVP_CIPHER_CTX_set_padding((EVP_CIPHER_CTX *)context, 0);
}

size_t Cipher::put(const unsigned char *data, size_t size)
{
    int outlen;
    size_t count = 0;

    if(!bufaddr)
        return 0;

    if(size % keys.iosize())
        return 0;

    while(bufsize && size + bufpos > bufsize) {
        size_t diff = bufsize - bufpos;
        count += put(data, diff);
        data += diff;
        size -= diff;
    }

    if(!EVP_CipherUpdate((EVP_CIPHER_CTX *)context, bufaddr + bufpos, &outlen, data, size)) {
        release();
        return count;
    }
    bufpos += outlen;
    count += outlen;
    if(bufsize && bufpos >= bufsize) {
        push(bufaddr, bufsize);
        bufpos = 0;
    }
    return count;
}

size_t Cipher::pad(const unsigned char *data, size_t size)
{
    size_t padsize = 0;
    unsigned char padbuf[64];
    const unsigned char *ep;

    if(!bufaddr)
        return 0;

    switch(bufmode) {
    case DECRYPT:
        if(size % keys.iosize())
            return 0;
        put(data, size);
        ep = data + size - 1;
        bufpos -= *ep;
        size -= *ep;
        break;
    case ENCRYPT:
        padsize = size % keys.iosize();
        put(data, size - padsize);
        if(padsize) {
            memcpy(padbuf, data + size - padsize, padsize);
            memset(padbuf + padsize, keys.iosize() - padsize, keys.iosize() - padsize);
            size = (size - padsize) + keys.iosize();
        }
        else {
            size += keys.iosize();
            memset(padbuf, keys.iosize(), keys.iosize());
        }

        put((const unsigned char *)padbuf, keys.iosize());
        zerofill(padbuf, sizeof(padbuf));
    }

    flush();
    return size;
}

size_t Cipher::process(unsigned char *buf, size_t len, bool flag)
{
    set(buf);
    if(flag)
        return pad(buf, len);
    else
        return put(buf, len);
}

