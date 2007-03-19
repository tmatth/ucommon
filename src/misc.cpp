#include <private.h>
#include <ucommon/misc.h>
#include <ucommon/string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>

#if HAVE_ENDIAN_H
#include <endian.h>
#else
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#define __BIG_ENDIAN 4321
#endif

#ifndef __BYTE_ORDER
#define __BYTE_ORDER 1234
#endif

#endif

struct MD5Context {
	uint32_t buf[4];
	uint32_t bits[2];
	unsigned char in[64];
};

static void MD5Transform(uint32_t buf[4], uint32_t const in[16]);

#if(__BYTE_ORDER == __LITTLE_ENDIAN)
#define byteReverse(buf, len)	/* Nothing */
#else
static void byteReverse(unsigned char *buf, unsigned longs)
{
	uint32_t t;
	do {
	t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
	    ((unsigned) buf[1] << 8 | buf[0]);
	*(uint32_t *) buf = t;
	buf += 4;
	} while (--longs);
}
#endif

static void MD5Init(struct MD5Context *ctx)
{
	ctx->buf[0] = 0x67452301;
	ctx->buf[1] = 0xefcdab89;
	ctx->buf[2] = 0x98badcfe;
	ctx->buf[3] = 0x10325476;

	ctx->bits[0] = 0;
	ctx->bits[1] = 0;
}

static void MD5Update(struct MD5Context *ctx, unsigned char const *buf, unsigned len)
{
	uint32_t t;

	/* Update bitcount */

	t = ctx->bits[0];
	if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
	ctx->bits[1]++;		/* Carry from low to high */
	ctx->bits[1] += len >> 29;

	t = (t >> 3) & 0x3f;	/* Bytes already in shsInfo->data */

	/* Handle any leading odd-sized chunks */

	if (t) {
	unsigned char *p = (unsigned char *) ctx->in + t;

	t = 64 - t;
	if (len < t) {
	    memcpy(p, buf, len);
	    return;
	}
	memcpy(p, buf, t);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += t;
	len -= t;
	}
	/* Process data in 64-byte chunks */

	while (len >= 64) {
	memcpy(ctx->in, buf, 64);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	buf += 64;
	len -= 64;
	}

	/* Handle any remaining bytes of data. */

	memcpy(ctx->in, buf, len);
}

static void MD5Final(unsigned char digest[16], struct MD5Context *ctx)
{
	unsigned count;
	unsigned char *p;

	/* Compute number of bytes mod 64 */
	count = (ctx->bits[0] >> 3) & 0x3F;

	/* Set the first char of padding to 0x80.  This is safe since there is
	   always at least one byte free */
	p = ctx->in + count;
	*p++ = 0x80;

	/* Bytes of padding needed to make 64 bytes */
	count = 64 - 1 - count;

	/* Pad out to 56 mod 64 */
	if (count < 8) {
	/* Two lots of padding:  Pad the first block to 64 bytes */
	memset(p, 0, count);
	byteReverse(ctx->in, 16);
	MD5Transform(ctx->buf, (uint32_t *) ctx->in);

	/* Now fill the next block with 56 bytes */
	memset(ctx->in, 0, 56);
	} else {
	/* Pad block to 56 bytes */
	memset(p, 0, count - 8);
	}
	byteReverse(ctx->in, 14);

	/* Append length in bits and transform */
	((uint32_t *) ctx->in)[14] = ctx->bits[0];
	((uint32_t *) ctx->in)[15] = ctx->bits[1];

	MD5Transform(ctx->buf, (uint32_t *) ctx->in);
	byteReverse((unsigned char *) ctx->buf, 4);
	memcpy(digest, ctx->buf, 16);
	memset(ctx, 0, sizeof(ctx));	/* In case it's sensitive */
}

#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

#define MD5STEP(f, w, x, y, z, data, s) \
	( w += f /*(x, y, z)*/ + data,  w = w<<s | w>>(32-s),  w += x )

static void MD5Transform(uint32_t buf[4], uint32_t const in[16])
{
	register uint32_t a, b, c, d;

	a = buf[0];
	b = buf[1];
	c = buf[2];
	d = buf[3];

	MD5STEP(F1(b,c,d), a, b, c, d, in[0] + 0xd76aa478L, 7);
	MD5STEP(F1(a,b,c), d, a, b, c, in[1] + 0xe8c7b756L, 12);
	MD5STEP(F1(d,a,b), c, d, a, b, in[2] + 0x242070dbL, 17);
	MD5STEP(F1(c,d,a), b, c, d, a, in[3] + 0xc1bdceeeL, 22);
	MD5STEP(F1(b,c,d), a, b, c, d, in[4] + 0xf57c0fafL, 7);
	MD5STEP(F1(a,b,c), d, a, b, c, in[5] + 0x4787c62aL, 12);
	MD5STEP(F1(d,a,b), c, d, a, b, in[6] + 0xa8304613L, 17);
	MD5STEP(F1(c,d,a), b, c, d, a, in[7] + 0xfd469501L, 22);
	MD5STEP(F1(b,c,d), a, b, c, d, in[8] + 0x698098d8L, 7);
	MD5STEP(F1(a,b,c), d, a, b, c, in[9] + 0x8b44f7afL, 12);
	MD5STEP(F1(d,a,b), c, d, a, b, in[10] + 0xffff5bb1L, 17);
	MD5STEP(F1(c,d,a), b, c, d, a, in[11] + 0x895cd7beL, 22);
	MD5STEP(F1(b,c,d), a, b, c, d, in[12] + 0x6b901122L, 7);
	MD5STEP(F1(a,b,c), d, a, b, c, in[13] + 0xfd987193L, 12);
	MD5STEP(F1(d,a,b), c, d, a, b, in[14] + 0xa679438eL, 17);
	MD5STEP(F1(c,d,a), b, c, d, a, in[15] + 0x49b40821L, 22);

	MD5STEP(F2(b,c,d), a, b, c, d, in[1] + 0xf61e2562L, 5);
	MD5STEP(F2(a,b,c), d, a, b, c, in[6] + 0xc040b340L, 9);
	MD5STEP(F2(d,a,b), c, d, a, b, in[11] + 0x265e5a51L, 14);
	MD5STEP(F2(c,d,a), b, c, d, a, in[0] + 0xe9b6c7aaL, 20);
	MD5STEP(F2(b,c,d), a, b, c, d, in[5] + 0xd62f105dL, 5);
	MD5STEP(F2(a,b,c), d, a, b, c, in[10] + 0x02441453L, 9);
	MD5STEP(F2(d,a,b), c, d, a, b, in[15] + 0xd8a1e681L, 14);
	MD5STEP(F2(c,d,a), b, c, d, a, in[4] + 0xe7d3fbc8L, 20);
	MD5STEP(F2(b,c,d), a, b, c, d, in[9] + 0x21e1cde6L, 5);
	MD5STEP(F2(a,b,c), d, a, b, c, in[14] + 0xc33707d6L, 9);
	MD5STEP(F2(d,a,b), c, d, a, b, in[3] + 0xf4d50d87L, 14);
	MD5STEP(F2(c,d,a), b, c, d, a, in[8] + 0x455a14edL, 20);
	MD5STEP(F2(b,c,d), a, b, c, d, in[13] + 0xa9e3e905L, 5);
	MD5STEP(F2(a,b,c), d, a, b, c, in[2] + 0xfcefa3f8L, 9);
	MD5STEP(F2(d,a,b), c, d, a, b, in[7] + 0x676f02d9L, 14);
	MD5STEP(F2(c,d,a), b, c, d, a, in[12] + 0x8d2a4c8aL, 20);

	MD5STEP(F3(b,c,d), a, b, c, d, in[5] + 0xfffa3942L, 4);
	MD5STEP(F3(a,b,c), d, a, b, c, in[8] + 0x8771f681L, 11);
	MD5STEP(F3(d,a,b), c, d, a, b, in[11] + 0x6d9d6122L, 16);
	MD5STEP(F3(c,d,a), b, c, d, a, in[14] + 0xfde5380cL, 23);
	MD5STEP(F3(b,c,d), a, b, c, d, in[1] + 0xa4beea44L, 4);
	MD5STEP(F3(a,b,c), d, a, b, c, in[4] + 0x4bdecfa9L, 11);
	MD5STEP(F3(d,a,b), c, d, a, b, in[7] + 0xf6bb4b60L, 16);
	MD5STEP(F3(c,d,a), b, c, d, a, in[10] + 0xbebfbc70L, 23);
	MD5STEP(F3(b,c,d), a, b, c, d, in[13] + 0x289b7ec6L, 4);
	MD5STEP(F3(a,b,c), d, a, b, c, in[0] + 0xeaa127faL, 11);
	MD5STEP(F3(d,a,b), c, d, a, b, in[3] + 0xd4ef3085L, 16);
	MD5STEP(F3(c,d,a), b, c, d, a, in[6] + 0x04881d05L, 23);
	MD5STEP(F3(b,c,d), a, b, c, d, in[9] + 0xd9d4d039L, 4);
	MD5STEP(F3(a,b,c), d, a, b, c, in[12] + 0xe6db99e5L, 11);
	MD5STEP(F3(d,a,b), c, d, a, b, in[15] + 0x1fa27cf8L, 16);
	MD5STEP(F3(c,d,a), b, c, d, a, in[2] + 0xc4ac5665L, 23);

	MD5STEP(F4(b,c,d), a, b, c, d, in[0] + 0xf4292244L, 6);
	MD5STEP(F4(a,b,c), d, a, b, c, in[7] + 0x432aff97L, 10);
	MD5STEP(F4(d,a,b), c, d, a, b, in[14] + 0xab9423a7L, 15);
	MD5STEP(F4(c,d,a), b, c, d, a, in[5] + 0xfc93a039L, 21);
	MD5STEP(F4(b,c,d), a, b, c, d, in[12] + 0x655b59c3L, 6);
	MD5STEP(F4(a,b,c), d, a, b, c, in[3] + 0x8f0ccc92L, 10);
	MD5STEP(F4(d,a,b), c, d, a, b, in[10] + 0xffeff47dL, 15);
	MD5STEP(F4(c,d,a), b, c, d, a, in[1] + 0x85845dd1L, 21);
	MD5STEP(F4(b,c,d), a, b, c, d, in[8] + 0x6fa87e4fL, 6);
	MD5STEP(F4(a,b,c), d, a, b, c, in[15] + 0xfe2ce6e0L, 10);
	MD5STEP(F4(d,a,b), c, d, a, b, in[6] + 0xa3014314L, 15);
	MD5STEP(F4(c,d,a), b, c, d, a, in[13] + 0x4e0811a1L, 21);
	MD5STEP(F4(b,c,d), a, b, c, d, in[4] + 0xf7537e82L, 6);
	MD5STEP(F4(a,b,c), d, a, b, c, in[11] + 0xbd3af235L, 10);
	MD5STEP(F4(d,a,b), c, d, a, b, in[2] + 0x2ad7d2bbL, 15);
	MD5STEP(F4(c,d,a), b, c, d, a, in[9] + 0xeb86d391L, 21);

	buf[0] += a;
	buf[1] += b;
	buf[2] += c;
	buf[3] += d;
}

extern "C" void cpr_md5hash(char *out, const char *source, size_t len)
{
	struct MD5Context md5;
	unsigned char digest[16];
	unsigned p = 0;

	assert(out != NULL && source != NULL && len != 0);

	if(!len)
		len = cpr_strlen(source);

	MD5Init(&md5);
	MD5Update(&md5, (unsigned char *)source, len);
	MD5Final(digest, &md5);

	while(p < 16) {
		snprintf(&out[p * 2], 3, "%2.2x", digest[p]);
		++p;
	}
}

extern "C" int cpr_uuid(char *uuid)
{
	int fd = open("/dev/urandom", O_RDONLY);
	char buf[16];
	if(fd < 0)
		return -1;

	if(read(fd, buf, 16) < 16)
		return -1;

	assert(uuid != NULL);

	snprintf(uuid, uuid_size, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		buf[0], buf[1], buf[2], buf[3],
        buf[4], buf[5], buf[6], buf[7],
        buf[8], buf[9], buf[10], buf[11],
        buf[12], buf[13], buf[14], buf[15]);

	close(fd);
	return 0;
}

extern "C" size_t cpr_urlencodesize(char *src)
{
	size_t size = 0;

	while(src && *src) {
		if(isalnum(*src) ||  strchr("/.-:;, ", *src))
			++size;
		else
			size += 3;
	}
	return size;
}

extern "C" size_t cpr_urlencode(char *dest, size_t limit, const char *src)
{
	static const char *hex = "0123456789abcdef";
	char *ret = dest;
	unsigned char ch;

	assert(dest != NULL && src != NULL && limit > 3);
	
	while(limit-- > 3 && *src) {
		if(*src == ' ') {
			*(dest++) = '+';
			++src;
		}
		else if(isalnum(*src) ||  strchr("/.-:;,", *src))
			*(dest++) = *(src++);
		else {		
			limit -= 2;
			ch = (unsigned char)(*(src++));
			*(dest++) = '%';
			*(dest++) = hex[(ch >> 4)&0xF];
			*(dest++) = hex[ch % 16];
		}
	}		
	*dest = 0;
	return dest - ret;
}

extern "C" size_t cpr_urldecode(char *dest, size_t limit, const char *src)
{
	char *ret = dest;
	char hex[3];

	assert(dest != NULL && src != NULL && limit > 1);

	while(limit-- > 1 && *src && !strchr("?&", *src)) {
		if(*src == '%') {
			memset(hex, 0, 3);
			hex[0] = *(++src);
			if(*src)
				hex[1] = *(++src);
			if(*src)
				++src;			
			*(dest++) = (char)strtol(hex, NULL, 16);	
		}
		else if(*src == '+') {
			*(dest++) = ' ';
			++src;
		}
		else
			*(dest++) = *(src++);
	}
	*dest = 0;
	return dest - ret;
}

extern "C" size_t cpr_xmlencode(char *out, size_t limit, const char *src)
{
	char *ret = out;

	assert(src != NULL && out != NULL && limit > 0);

	while(src && *src && limit > 6) {
		switch(*src) {
		case '<':
			strcpy(out, "&lt;");
			limit -= 4;
			out += 4;
			break;
		case '>':
			strcpy(out, "&gt;");
			limit -= 4;
			out += 4;
			break;
		case '&':
			strcpy(out, "&amp;");
			limit -= 5;
			out += 5;
			break;
		case '\"':
			strcpy(out, "&quot;");
			limit -= 6;
			out += 6;
			break;
		case '\'':
			strcpy(out, "&apos;");
			limit -= 6;
			out += 6;
			break;
		default:
			--limit;
			*(out++) = *src;
		}
		++src;
	}
	*out = 0;
	return out - ret;
}

extern "C" size_t cpr_xmldecode(char *out, size_t limit, const char *src)
{
	char *ret = out;

	assert(src != NULL && out != NULL && limit > 0);

	if(*src == '\'' || *src == '\"')
		++src;
	while(src && limit-- > 1 && !strchr("<\'\">", *src)) {
		if(!cpr_strnicmp(src, "&amp;", 5)) {
			*(out++) = '&';
			src += 5;
		}
		else if(!cpr_strnicmp(src, "&lt;", 4)) {
			src += 4;
			*(out++) = '<';
		}
		else if(!cpr_strnicmp(src, "&gt;", 4)) {
			src += 4;
			*(out++) = '>';
		}
		else if(!cpr_strnicmp(src, "&quot;", 6)) {
			src += 6;
			*(out++) = '\"';
		}
		else if(!cpr_strnicmp(src, "&apos;", 6)) {
			src += 6;
			*(out++) = '\'';
		}
		else
			*(out++) = *(src++);
	}
	*out = 0;
	return out - ret;
}

extern "C" size_t cpr_snprintf(char *out, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	assert(fmt != NULL && size > 0 && out != NULL);

	if(out)
		vsnprintf(out, size, fmt, args);

	va_end(args);
	return cpr_strlen(out);
}

static const unsigned char alphabet[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

extern "C" size_t cpr_b64decode(caddr_t out, const char *src, size_t count)
{
	static char *decoder = NULL;
	int i, bits, c;

	unsigned char *dest = (unsigned char *)out;

	if(!decoder) {
		decoder = (char *)malloc(256);
		for (i = 0; i < 256; ++i)
			decoder[i] = 64;
		for (i = 0; i < 64 ; ++i)
			decoder[alphabet[i]] = i;
	}

	bits = 1;

	while(*src) {
		c = (unsigned char)(*(src++));
		if (c == '=') {
			if (bits & 0x40000) {
				if (count < 2) break;
				*(dest++) = (bits >> 10);
				*(dest++) = (bits >> 2) & 0xff;
				break;
			}
			if (bits & 0x1000 && count)
				*(dest++) = (bits >> 4);
			break;
		}
		// skip invalid chars
		if (decoder[c] == 64)
			continue;
		bits = (bits << 6) + decoder[c];
		if (bits & 0x1000000) {
			if (count < 3) break;
			*(dest++) = (bits >> 16);
			*(dest++) = (bits >> 8) & 0xff;
			*(dest++) = (bits & 0xff);
		    	bits = 1;
			count -= 3;
		}
	}
	return (size_t)((caddr_t)dest - out);
}

extern "C" size_t cpr_b64encode(char *out, caddr_t src, size_t count)
{
	unsigned bits;
	char *dest = out;

	assert(out != NULL && src != NULL && count > 0);

	while(count >= 3) {
		bits = (((unsigned)src[0])<<16) | (((unsigned)src[1])<<8)
			| ((unsigned)src[2]);
		src += 3;
		count -= 3;
		*(out++) = alphabet[bits >> 18];
	    *(out++) = alphabet[(bits >> 12) & 0x3f];
	    *(out++) = alphabet[(bits >> 6) & 0x3f];
	    *(out++) = alphabet[bits & 0x3f];
	}
	if (count) {
		bits = ((unsigned)src[0])<<16;
		*(out++) = alphabet[bits >> 18];
		if (count == 1) {
			*(out++) = alphabet[(bits >> 12) & 0x3f];
	    		*(out++) = '=';
		}
		else {
			bits |= ((unsigned)src[1])<<8;
			*(out++) = alphabet[(bits >> 12) & 0x3f];
	    		*(out++) = alphabet[(bits >> 6) & 0x3f];
		}
	    	*(out++) = '=';
	}
	*out = 0;
	return out - dest;
}

extern "C" size_t cpr_b64len(const char *str)
{
	unsigned count = cpr_strlen(str);
	const char *ep = str + count - 1;

	if(!count)
		return 0;

	count /= 4;
	count *= 3;
	if(*ep == '=') {
		--ep;
		--count;
		if(*ep == '=')
			--count;
	}
	return count;
}

#ifdef	_MSWINDOWS_
extern "C" void cpr_printlog(const char *path, const char *fmt, ...)
{
	char buffer[256];
	char *ep;
	FILE *fp;
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
	ep = buffer + strlen(buffer);
    if(ep > buffer) {
        --ep;
        if(*ep != '\n') {
            *(++ep) = '\n';
            *ep = 0;
        }
    }
	fp = fopen(path, "a+");
	assert(fp != NULL);
	if(!fp)
		return; 
	fprintf(fp, "%s\n", buffer);
	fclose(fp);
}

#else
extern "C" void cpr_printlog(const char *path, const char *fmt, ...)
{
	char buffer[256];
	char *ep;
	int fd;
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
	ep = buffer + strlen(buffer);
	if(ep > buffer) {
		--ep;
		if(*ep != '\n') {
			*(++ep) = '\n';
			*ep = 0;
		}
	}
	fd = open(path, O_CREAT | O_WRONLY | O_APPEND, 0770);
	assert(fd > -1);
	if(fd < 0)
		return;

	write(fd, buffer, strlen(buffer));
	fsync(fd);
	close(fd);
	va_end(args);
}
#endif

// vim: set ts=4 sw=4:

