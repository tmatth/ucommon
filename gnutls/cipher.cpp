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

Cipher::Key::Key(const char *cipher, const char *digest, const char *text, size_t size, const unsigned char *salt, unsigned count)
{
	if(ieq(digest, "sha"))
		digest = "sha1";

    char algoname[64];

    String::set(algoname, sizeof(algoname), cipher);
    char *fpart = strchr(algoname, '-');
    char *lpart = strrchr(algoname, '-');

	modeid = GCRY_CIPHER_MODE_CBC;

	if(lpart && lpart != fpart) {
		*(lpart++) = 0;
		if(ieq(lpart, "cbc"))
			modeid = GCRY_CIPHER_MODE_CBC;
		else if(ieq(lpart, "ecb"))
			modeid = GCRY_CIPHER_MODE_ECB;
		else if(ieq(lpart, "cfb"))
			modeid = GCRY_CIPHER_MODE_CFB;
		else if(ieq(lpart, "ofb"))
			modeid = GCRY_CIPHER_MODE_OFB;
		else
			modeid = GCRY_CIPHER_MODE_NONE;
	}

	algoid = gcry_cipher_map_name(algoname);
	hashid = gcry_md_map_name(digest);

	memset(keybuf, 0, sizeof(keybuf));
	memset(ivbuf, 0, sizeof(ivbuf));
	keysize = blksize = 0;

    if(hashid == GCRY_MD_NONE || algoid == GCRY_CIPHER_NONE)
        return;

	(void)gcry_cipher_algo_info(algoid, GCRYCTL_GET_KEYLEN, NULL, &keysize);
	(void)gcry_cipher_algo_info(algoid, GCRYCTL_GET_BLKLEN, NULL, &blksize);

	gcry_md_hd_t mdc;
	if(gcry_md_open(&mdc, hashid, 0) != 0)
		return;

	size_t kpos = 0, ivpos = 0;
	size_t mdlen = gcry_md_get_algo_dlen(hashid);
	size_t tlen = strlen(text);

	char previous[MAX_DIGEST_HASHSIZE / 8];
	unsigned prior = 0;
	unsigned loop;
	
	do {
		gcry_md_reset(mdc);

		if(prior++)
			gcry_md_write(mdc, previous, mdlen);

		gcry_md_write(mdc, (const char *)text, tlen);

		if(salt)
			gcry_md_write(mdc, (const char *)salt, 8);

		gcry_md_final(mdc);
		memcpy(previous, gcry_md_read(mdc, hashid), mdlen);

		for(loop = 1; loop < count; ++loop) {
			gcry_md_reset(mdc);
			gcry_md_write(mdc, previous, mdlen);
			gcry_md_final(mdc);
			memcpy(previous, gcry_md_read(mdc, hashid), mdlen);
		}

		size_t pos = 0;
		while(kpos < keysize && pos < mdlen)
			keybuf[kpos++] = previous[pos++];
		while(ivpos < blksize && pos < mdlen)
			ivbuf[ivpos++] = previous[pos++];
	} while(kpos < keysize || ivpos < blksize);
	gcry_md_close(mdc);
}

Cipher::Key::Key()
{
	algoid = GCRY_CIPHER_NONE;
	hashid = GCRY_MD_NONE;
	keysize = blksize = 0;
	memset(keybuf, 0, sizeof(keybuf));
	memset(ivbuf, 0, sizeof(ivbuf));
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

void Cipher::push(unsigned char *address, size_t size)
{
}

void Cipher::release(void)
{
	if(context) {
		gcry_cipher_close((CIPHER_CTX)context);
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
    return total;
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

	gcry_cipher_open((CIPHER_CTX *)&context, keys.algoid, keys.modeid,0);
	gcry_cipher_setkey((CIPHER_CTX)context, keys.keybuf, keys.keysize);
	gcry_cipher_setiv((CIPHER_CTX)context, keys.ivbuf, keys.blksize);
}

size_t Cipher::puts(const char *text)
{
    char padbuf[64];
    if(!text)
        return 0;

    size_t len = strlen(text) + 1;
    unsigned pad = len % keys.iosize();

    size_t count = put((const unsigned char *)text, len - pad);
    if(pad) {
        memcpy(padbuf, text + len - pad, pad);
        memset(padbuf + pad, 0, keys.iosize() - pad);
        count += put((const unsigned char *)padbuf, keys.iosize());
    }
    return flush();
}

size_t Cipher::put(const unsigned char *data, size_t size)
{
	gcry_error_t errcode;

	if(size % keys.iosize())
        return 0;

	size_t count = 0;

	while(bufsize && size + bufpos > bufsize) {
		size_t diff = bufsize - bufpos;
		count += put(data, diff);
		data += diff;
		size -= diff;
	}

	switch(bufmode) {
	case Cipher::ENCRYPT:
		errcode = gcry_cipher_encrypt((CIPHER_CTX)context, bufaddr + bufpos, size, data, size);
		break;
	case Cipher::DECRYPT:
		errcode = gcry_cipher_decrypt((CIPHER_CTX)context, bufaddr + bufpos, size, data, size);
	}

	count += size;
	if(!count) {
		release();
		return 0;
	}
	bufpos += size;
	if(bufsize && bufpos >= bufsize) {
		push(bufaddr, bufsize);
		bufpos = 0;
	}
	return count;
}

size_t Cipher::final(const unsigned char *data, size_t size)
{
	size_t pad = 0;
	unsigned char padbuf[64];
	const unsigned char *ep;

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
		pad = size % keys.iosize();
		put(data, size - pad);
		if(pad) {
			memcpy(padbuf, data + size - pad, pad);
			memset(padbuf + pad, keys.iosize() - pad, keys.iosize() - pad);
			size = (size - pad) + keys.iosize();
		}
		else {
			size += keys.iosize();
			memset(padbuf, keys.iosize(), keys.iosize());
		}

		put((const unsigned char *)padbuf, keys.iosize());
	}

	flush();
	return size;
}

