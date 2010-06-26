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

	algotype = EVP_get_cipherbyname(cipher);
	hashtype = EVP_get_digestbyname(digest);
	keysize = blksize = 0;
	memset(keybuf, 0, sizeof(keybuf));
	memset(ivbuf, 0, sizeof(ivbuf));

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

	algotype = NULL;
	hashtype = NULL;
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

bool Cipher::is(const char *id)
{
	return (EVP_get_cipherbyname(id) != NULL);
}

void Cipher::push(unsigned char *address, size_t size)
{
}

void Cipher::release(void)
{
	if(context) {
		EVP_CIPHER_CTX_cleanup((EVP_CIPHER_CTX*)context);
		delete (EVP_CIPHER_CTX*)context;
		context = NULL;
	}
}

size_t Cipher::flush(void)
{
	int outlen;

	if(context && EVP_CipherFinal_ex((EVP_CIPHER_CTX *)context, bufaddr + bufpos, &outlen)) {
		bufpos += outlen;
		if(bufpos >= bufsize) {
			push(bufaddr, bufsize);
			bufpos = 0;
		}
		return outlen;
	}
	return 0;
}

size_t Cipher::puts(const char *text)
{
	if(!text)
		return 0;

	return put((const unsigned char *)text, strlen(text));
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
}

size_t Cipher::put(const unsigned char *data, size_t size)
{
	int outlen;
	size_t count = 0;

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
