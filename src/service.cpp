#include <config.h>
#include <ucommon/string.h>
#include <ucommon/service.h>
#include <ucommon/socket.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>

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

#ifdef	HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#include <sys/types.h>
#endif

#if HAVE_FTOK
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <limits.h>

#ifndef	OPEN_MAX
#define	OPEN_MAX 20
#endif

#if defined(_MSWINDOWS_)

static bool createpipe(fd_t *fd, size_t size)
{
	if(CreatePipe(&fd[0], &fd[1], NULL, size)) {
		fd[0] = fd[1] = INVALID_HANDLE_VALUE;
		return false;
	}
	return true;
}

#elif defined(HAVE_SOCKETPAIR) && defined(AF_UNIX) && defined(SO_RCVBUF)

static bool createpipe(fd_t *fd, size_t size)
{
	if(!size) {
		if(pipe(fd) == 0)
			return true;
		fd[0] = fd[1] = INVALID_HANDLE_VALUE;
		return false;
	}

	if(socketpair(AF_UNIX, SOCK_STREAM, 0, fd)) {
		fd[0] = fd[1] = INVALID_HANDLE_VALUE;
		return false;
	}

	shutdown(fd[1], SHUT_RD);
	shutdown(fd[0], SHUT_WR);
	setsockopt(fd[0], SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
	return true;
}

#else

static bool createpipe(fd_t *fd, size_t size)
{
	return pipe(fd) == 0;
}
#endif

#if	defined(HAVE_FTOK) && !defined(HAVE_SHM_OPEN)

#include <sys/ipc.h>
#include <sys/shm.h>

static void ftok_name(const char *name, char *buf, size_t max)
{
	if(*name == '/')
		++name;

	if(cpr_isdir("/var/run/ipc"))
		snprintf(buf, sizeof(buf), "/var/run/ipc/%s", name);
	else
		snprintf(buf, sizeof(buf), "/tmp/.%s.ipc", name);
}

static key_t createipc(const char *name, char mode)
{
	char buf[65];
	int fd;

	ftok_name(name, buf, sizeof(buf));
	fd = open(buf, O_CREAT | O_EXCL | O_WRONLY, 0660);
	if(fd > -1)
		close(fd);
	return ftok(buf, mode);
}

static key_t accessipc(const char *name, char mode)
{
	char buf[65];

	ftok_name(name, buf, sizeof(buf));
	return ftok(buf, mode);
}

#endif

using namespace UCOMMON_NAMESPACE;

OrderedIndex config::callback::list;
SharedLock config::lock;
config *config::cfg = NULL;

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

void service::md5hash(char *out, uint8_t *source, size_t len)
{
	struct MD5Context md5;
	unsigned char digest[16];
	unsigned p = 0;

	assert(out != NULL && source != NULL && len != 0);

	if(!len)
		len = strlen((char *)source);

	MD5Init(&md5);
	MD5Update(&md5, (unsigned char *)source, len);
	MD5Final(digest, &md5);

	while(p < 16) {
		snprintf(&out[p * 2], 3, "%2.2x", digest[p]);
		++p;
	}
}

int service::uuid(char *uuid)
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

config::callback::callback() :
OrderedObject(&list)
{
}

config::callback::~callback()
{
	release();
}

void config::callback::release(void)
{
	delist(&list);
}

void config::callback::reload(config *keys)
{
}

void config::callback::commit(config *keys)
{
}

config::instance::instance()
{
	config::protect();
}

config::instance::~instance()
{
	config::release();
}

config::config(char *name, size_t s) :
mempager(s), root()
{
	root.setId(name);
}

config::~config()
{
	mempager::purge();
}

config::keynode *config::getPath(const char *id)
{
	const char *np;
	char buf[65];
	char *ep;
	keynode *node = &root, *child;
	caddr_t mp;

	while(id && *id && node) {
		string::set(buf, sizeof(buf), id);
		ep = strchr(buf, '.');
		if(ep)
			*ep = 0;
		np = strchr(id, '.');
		if(np)
			id = ++np;
		else
			id = NULL;
		child = node->getChild(buf);
		if(!child) {
			mp = (caddr_t)alloc(sizeof(keynode));
			child = new(mp) keynode(node, dup(buf));
		}
		node = child;			
	}
	return node;
}

config::keynode *config::addNode(keynode *base, const char *id, const char *value)
{
	caddr_t mp;
	keynode *node;

	mp = (caddr_t)alloc(sizeof(keynode));
	node = new(mp) keynode(base, dup(id));
	if(value)
		node->setPointer(dup(value));
	return node;
}

const char *config::getValue(keynode *node, const char *id, keynode *alt)
{
	node = node->getChild(id);
	if(!node && alt)
		node = alt->getChild(id);

	if(!node)
		return NULL;

	return node->getPointer();
}

config::keynode *config::addNode(keynode *base, define *defs)
{
	keynode *node = getNode(base, defs->key, defs->value);
	if(!node)
		node = addNode(base, defs->key, defs->value);

	for(;;) {
		++defs;
		if(!defs->key)
			return base;
		if(node->getChild(defs->key))
			continue;
		addNode(node, defs->key, defs->value);
	}
	return node;
}


config::keynode *config::getNode(keynode *base, const char *id, const char *text)
{
	linked_pointer<keynode> node = base->getFirst();
	char *cp;
	
	while(node) {
		if(!strcmp(id, node->getId())) {
			cp = node->getData();
			if(cp && !stricmp(cp, text))
				return *node;
		}
		node.next();
	}
	return NULL;
} 

bool config::loadconf(const char *fn, keynode *node, char *gid, keynode *entry)
{
	FILE *fp = fopen(fn, "r");
	bool rtn = false;
	keynode *data;
	caddr_t mp;
	const char *cp;
	char *value;

	if(!node)
		node = &root;

	if(!fp)
		return false;

	while(!feof(fp)) {
		buffer << fp;
		buffer.strip(" \t\r\n");
		if(buffer[0] == '[') {
			if(!buffer.unquote("[]"))
				goto exit;
			value = mempager::dup(*buffer);
			if(gid)
				entry = getNode(node, gid, value);
			else
				entry = node->getChild(value);
			if(!entry) {
				mp = (caddr_t)alloc(sizeof(keynode));
				if(gid) {
					entry = new(mp) keynode(node, gid);
					entry->setPointer(value);
				}
				else					
					entry = new(mp) keynode(node, value);
			}
		}
		if(!buffer[0] || !isalnum(buffer[0]))
			continue;	
		if(!entry)
			continue;
		cp = buffer.chr('=');
		if(!cp)
			continue;
		buffer.split(cp++);
		buffer.chop(" \t=");
		while(isspace(*cp))
			++cp;
		data = entry->getChild(buffer.c_mem());
		if(!data) {
			mp = (caddr_t)alloc(sizeof(keynode));
			data = new(mp) keynode(entry, mempager::dup(*buffer));
		}
		data->setPointer(mempager::dup(cp));
	}

	rtn = true;
exit:
	fclose(fp);
	return rtn;
}

bool config::loadxml(const char *fn, keynode *node)
{
	FILE *fp = fopen(fn, "r");
	char *cp, *ep, *bp, *id;
	ssize_t len = 0;
	bool rtn = false;
	bool document = false, empty;
	keynode *match;

	if(!node)
		node = &root;

	if(!fp)
		return false;

	buffer = "";

	while(node) {
		cp = buffer.c_mem() + buffer.len();
		if(buffer.len() < 1024 - 5) {
			len = fread(cp, 1, 1024 - buffer.len() - 1, fp);
		}
		else
			len = 0;

		if(len < 0)
			goto exit;
		cp[len] = 0;
		if(!buffer.chr('<'))
			goto exit;
		buffer = buffer.c_str();
		cp = buffer.c_mem();

		while(node && cp && *cp)
		{
			cp = string::trim(cp, " \t\r\n");

			if(cp && *cp && !node)
				goto exit;

			bp = strchr(cp, '<');
			ep = strchr(cp, '>');
			if(!ep && bp == cp)
				break;
			if(!bp ) {
				cp = cp + strlen(cp);
				break;
			}
			if(bp > cp) {
				if(node->getData() != NULL)
					goto exit;
				*bp = 0;
				cp = string::chop(cp, " \r\n\t");
				len = strlen(cp);
				ep = (char *)alloc(len + 1);
				string::xmldecode(ep, len, cp);
				node->setPointer(ep);
				*bp = '<';
				cp = bp;
				continue;
			}

			empty = false;	
			*ep = 0;
			if(*(ep - 1) == '/') {
				*(ep - 1) = 0;
				empty = true;
			}
			cp = ++ep;

			if(!strncmp(bp, "</", 2)) {
				if(strcmp(bp + 2, node->getId()))
					goto exit;

				node = node->getParent();
				continue;
			}		

			++bp;

			// if comment/control field...
			if(!isalnum(*bp))
				continue;

			ep = bp;
			while(isalnum(*ep))
				++ep;

			id = NULL;
			if(isspace(*ep)) {
				*(ep++) = 0;
				id = strchr(ep, '\"');
			}
			else if(*ep)
				goto exit;

			if(id) {
				ep = strchr(id + 1, *id);
				if(!ep)
					goto exit;
				*ep = 0;
				++id;
			}

			if(!document) {
				if(strcmp(node->getId(), bp))
					goto exit;
				document = true;
				continue;
			}

			if(id)
				match = getNode(node, bp, id);
			else
				match = node->getChild(bp);
			if(match) {
				if(!id)
					match->setPointer(NULL);
				node = match;
			}
			else 
				node = addNode(node, bp, id);

			if(empty)
				node = node->getParent();
		}
		buffer = cp;
	}
	if(!node && root.getId())
		rtn = true;
exit:
	fclose(fp);
	return rtn;
}

void config::commit(void)
{
	cb = callback::list.begin();
	while(cb) {
		cb->reload(this);
		cb.next();
	}
	lock.lock();

	cb = callback::list.begin();
	while(cb) {
		cb->commit(this);
		cb.next();
	}
	cfg = this;
	lock.unlock();
}

void config::update(void)
{
	lock.lock();
	if(cb)
		cb.next();

	while(cb) {
		cb->reload(this);
		cb->commit(this);
		cb.next();
	}
	lock.unlock();
}

#if defined(_MSWINDOWS_)

MappedMemory::MappedMemory(const char *fn, size_t len)
{
	int share = FILE_SHARE_READ;
	int prot = FILE_MAP_READ;
	int mode = GENERIC_READ;
	struct stat ino;

	size = 0;
	used = 0;
	map = NULL;

	if(len) {
		prot = FILE_MAP_WRITE;
		mode |= GENERIC_WRITE;
		share |= FILE_SHARE_WRITE;
		fd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, len, fn + 1);

	}
	else
		fd = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, fn + 1);
	
	if(fd == INVALID_HANDLE_VALUE || fd == NULL) 
		return;

	map = (caddr_t)MapViewOfFile(fd, FILE_MAP_ALL_ACCESS, 0, 0, len);
	if(map) {
		size = len;
		VirtualLock(map, size);
	}
}

MappedMemory::~MappedMemory()
{
	if(map) {
		VirtualUnlock(map, size);
		UnmapViewOfFile(fd);
		CloseHandle(fd);
		map = NULL;
		fd = INVALID_HANDLE_VALUE;
	}
}

#elif defined(HAVE_SHM_OPEN)

MappedMemory::MappedMemory(const char *fn, size_t len)
{
	int prot = PROT_READ;
	struct stat ino;

	size = 0;
	used = 0;
	
	if(len) {
		prot |= PROT_WRITE;
		shm_unlink(fn);
		fd = shm_open(fn, O_RDWR | O_CREAT, 0660);
		if(fd > -1)
			ftruncate(fd, len);
	}
	else {
		fd = shm_open(fn, O_RDONLY, 0660);
		if(fd > -1) {
			fstat(fd, &ino);
			len = ino.st_size;
		}
	}

	if(fd < 0)
		return;

	map = (caddr_t)mmap(NULL, len, prot, MAP_SHARED, fd, 0);
	close(fd);
	if(map != (caddr_t)MAP_FAILED) {
		size = len;
		mlock(map, size);
	}
}

MappedMemory::~MappedMemory()
{
	if(size) {
		munlock(map, size);
		munmap(map, size);
		size = 0;
	}
}

#else

MappedMemory::MappedMemory(const char *name, size_t len)
{
	struct shmid_ds stat;
	size = 0;
	used = 0;
	key_t key;

	if(len) {
		key = createipc(name, 'S');
remake:
		fd = shmget(key, len, IPC_CREAT | IPC_EXCL | 0660);
		if(fd == -1 && errno == EEXIST) {
			fd = shmget(key, 0, 0);
			if(fd > -1) {
				shmctl(fd, IPC_RMID, NULL);
				goto remake;
			}
		}
	}
	else {
		key = accessipc(name, 'S');
		fd = shmget(key, 0, 0);
	}
	
	if(fd > -1) {
		if(len)
			size = len;
		else if(shmctl(fd, IPC_STAT, &stat) == 0)
			size = stat.shm_segsz;
		else
			fd = -1;
	}
	map = (caddr_t)shmat(fd, NULL, 0);
#ifdef	SHM_LOCK
	if(fd > -1)
		shmctl(fd, SHM_LOCK, NULL);
#endif
}

MappedMemory::~MappedMemory()
{
	if(size > 0) {
#ifdef	SHM_UNLOCK
		shmctl(fd, SHM_UNLOCK, NULL);
#endif
		shmdt(map);
		fd = -1;
		size = 0;
	}
}

#endif

void MappedMemory::fault(void) 
{
	abort();
}

void *MappedMemory::sbrk(size_t len)
{
	void *mp = (void *)(map + used);
	if(used + len > size)
		fault();
	used += len;
	return mp;
}
	
void *MappedMemory::get(size_t offset)
{
	if(offset >= size)
		fault();
	return (void *)(map + offset);
}

MappedReuse::MappedReuse(const char *name, size_t osize, unsigned count) :
ReusableAllocator(), MappedMemory(name,  osize * count)
{
	objsize = osize;
}

bool MappedReuse::avail(void)
{
	bool rtn = false;
	lock();
	if(freelist || used < size)
		rtn = true;
	unlock();
	return rtn;
}

ReusableObject *MappedReuse::request(void)
{
    ReusableObject *obj = NULL;

	lock();
	if(freelist) {
		obj = freelist;
		freelist = next(obj);
	} 
	else if(used + objsize <= size)
		obj = (ReusableObject *)sbrk(objsize);
	unlock();
	return obj;	
}

ReusableObject *MappedReuse::get(void)
{
	return get(Timer::inf);
}

ReusableObject *MappedReuse::get(timeout_t timeout)
{
	bool rtn = true;
	Timer expires;
	ReusableObject *obj = NULL;

	if(timeout && timeout != Timer::inf)
		expires.set(timeout);

	lock();
	while(rtn && !freelist && used >= size) {
		++waiting;
		if(timeout == Timer::inf)
			wait();
		else if(timeout)
			rtn = wait(*expires);
		else
			rtn = false;
		--waiting;
	}
	if(!rtn) {
		unlock();
		return NULL;
	}
	if(freelist) {
		obj = freelist;
		freelist = next(obj);
	}
	else if(used + objsize <= size)
		obj = (ReusableObject *)sbrk(objsize);
	unlock();
	return obj;
}

service::service(size_t ps, define *def) :
mempager(ps)
{
	root = NULL;

	while(def && def->name) {
		set(def->name, def->value);
		++def;
	}
}

service::~service()
{
	purge();
}

void service::setenv(define *def)
{
	while(def && def->name) {
		setenv(def->name, def->value);
		++def;
	}
}

void service::setenv(const char *id, const char *value)
{
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_lock(&mutex);
#ifdef	HAVE_SETENV
	::setenv(id, value, 1);
#else
	char buf[128];
	snprintf(buf, sizeof(buf), "%s=%s", id, value);
	::putenv(buf);
#endif
	pthread_mutex_unlock(&mutex);
}

const char *service::get(const char *id)
{
	member *key = find(id);
	if(!key)
		return NULL;

	return key->value;
}

#ifdef	_MSWINDOWS_
char **service::getEnviron(void)
{
	char buf[1024 - 64];
	linked_pointer<member> env;
	unsigned idx = 0;
	unsigned count = LinkedObject::count(root) + 1;
	caddr_t mp = (caddr_t)alloc(count * sizeof(char *));
	char **envp = new(mp) char *[count];

	env = root;
	while(env) {
		snprintf(buf, sizeof(buf), "%s=%s", env->getId(), env->value);
		envp[idx++] = mempager::dup(buf);
	}
	envp[idx] = NULL;
	return envp;
}
#else

void service::setEnviron(void)
{
	const char *id;

	linked_pointer<service::member> env = begin();

	while(env) {
#ifdef	HAVE_SETENV
		::setenv(env->getId(), env->value, 1);
#else
		char buf[128];
		snprintf(buf, sizeof(buf), "%s=%s", env->getId(), env->value);
		putenv(buf);
#endif
		env.next();
	}

	if(getuid() == 0 && getppid() < 2)
		umask(002);

	id = get("UID");
	if(id && getuid() == 0) {
		if(getppid() > 1)
			umask(022);
		setuid(atoi(id));
	}

	id = get("HOME");
	if(id)
		chdir(id);

	id = get("PWD");
	if(id)
		chdir(id);	
}

pid_t service::pidfile(const char *id, pid_t pid)
{
	char buf[128];
	pid_t opid;
	fd_t fd;

	snprintf(buf, sizeof(buf), "/var/run/%s", id);
	mkdir(buf, 0775);
	if(cpr_isdir(buf))
		snprintf(buf, sizeof(buf), "/var/run/%s/%s.pid", id, id);
	else
		snprintf(buf, sizeof(buf), "/tmp/%s.pid", id);

retry:
	fd = open(buf, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0755);
	if(fd < 0) {
		opid = pidfile(id);
		if(!opid || opid == 1 && pid > 1) {
			remove(buf);
			goto retry;
		}
		return opid;
	}

	if(pid > 1) {
		snprintf(buf, sizeof(buf), "%d\n", pid);
		write(fd, buf, strlen(buf));
	}
	close(fd);
	return 0;
}

pid_t service::pidfile(const char *id)
{
	struct stat ino;
	time_t now;
	char buf[128];
	fd_t fd;
	pid_t pid;

	snprintf(buf, sizeof(buf), "/var/run/%s", id);
	if(cpr_isdir(buf))
		snprintf(buf, sizeof(buf), "/var/run/%s/%s.pid", id, id);
	else
		snprintf(buf, sizeof(buf), "/tmp/%s.pid", id);

	fd = open(buf, O_RDONLY);
	if(fd < 0 && errno == EPERM)
		return 1;

	if(fd < 0)
		return 0;

	if(read(fd, buf, 16) < 1) {
		goto bydate;
	}
	buf[16] = 0;
	pid = atoi(buf);
	if(pid == 1)
		goto bydate;

	close(fd);
	if(kill(pid, 0))
		return 0;

	return pid;

bydate:
	time(&now);
	fstat(fd, &ino);
	close(fd);
	if(ino.st_mtime + 30 < now)
		return 0;
	return 1;
}

bool service::reload(const char *id)
{
	pid_t pid = pidfile(id);

	if(pid < 2)
		return false;

	kill(pid, SIGHUP);
	return true;
}

bool service::shutdown(const char *id)
{
	pid_t pid = pidfile(id);

	if(pid < 2)
		return false;

	kill(pid, SIGINT);
	return true;
}

bool service::terminate(const char *id)
{
	pid_t pid = pidfile(id);

	if(pid < 2)
		return false;

	kill(pid, SIGTERM);
	return true;
}

#endif

void service::dup(const char *id, const char *value)
{
	member *env = find(id);

	if(!env) {
		caddr_t mp = (caddr_t)alloc(sizeof(member));
		env = new(mp) member(&root, mempager::dup(id));
	}
	env->value = mempager::dup(value);
};

void service::set(char *id, const char *value)
{
    member *env = find(id);
	
	if(!env) {
	    caddr_t mp = (caddr_t)alloc(sizeof(member));
	    env = new(mp) member(&root, id);
	}

    env->value = value;
};

#ifdef _MSWINDOWS_

#else

int service::spawn(const char *fn, char **args, int mode, pid_t *pid, fd_t *iov, service *env)
{
	unsigned max = OPEN_MAX, idx = 0;
	int status;

	*pid = fork();

	if(*pid < 0)
		return -1;

	if(*pid) {
		closeiov(iov);
		switch(mode) {
		case SPAWN_DETACH:
		case SPAWN_NOWAIT:
			return 0;
		case SPAWN_WAIT:
			cpr_waitpid(*pid, &status);
			return status;
		}
	}

	while(iov && *iov > -1) {
		if(*iov != (fd_t)idx)
			dup2(*iov, idx);
		++iov;
		++idx;
	}

	while(idx < 3)
		++idx;
	
#if defined(HAVE_SYSCONF)
	max = sysconf(_SC_OPEN_MAX);
#endif
#if defined(HAVE_SYS_RESOURCE_H)
	struct rlimit rl;
	if(!getrlimit(RLIMIT_NOFILE, &rl))
		max = rl.rlim_cur;
#endif

	closelog();

	while(idx < max)
		close(idx++);

	if(mode == SPAWN_DETACH)
		cpr_pdetach();

	if(env)
		env->setEnviron();

	execvp(fn, args);
	exit(-1);
}

void service::closeiov(fd_t *iov)
{
	unsigned idx = 0, np;

	while(iov && iov[idx] > -1) {
		if(iov[idx] != (fd_t)idx) {
			close(iov[idx]);
			np = idx;
			while(iov[++np] != -1) {
				if(iov[np] == iov[idx])
					iov[np] = (fd_t)np;
			}
		}
		++idx;
	}
}

void service::createiov(fd_t *fd)
{
	fd[0] = 0;
	fd[1] = 1;
	fd[2] = 2;
	fd[3] = INVALID_HANDLE_VALUE;
}

void service::attachiov(fd_t *fd, fd_t io)
{
	fd[0] = fd[1] = io;
	fd[2] = INVALID_HANDLE_VALUE;
}

void service::detachiov(fd_t *fd)
{
	fd[0] = open("/dev/null", O_RDWR);
	fd[1] = fd[2] = fd[0];
	fd[3] = INVALID_HANDLE_VALUE;
}

#endif

fd_t service::pipeInput(fd_t *fd, size_t size)
{
	fd_t pfd[2];

	if(!createpipe(pfd, size))
		return INVALID_HANDLE_VALUE;

	fd[0] = pfd[0];
	return pfd[1];
}

fd_t service::pipeOutput(fd_t *fd, size_t size)
{
	fd_t pfd[2];

	if(!createpipe(pfd, size))
		return INVALID_HANDLE_VALUE;

	fd[1] = pfd[1];
	return pfd[0];
}

fd_t service::pipeError(fd_t *fd, size_t size)
{
	return pipeOutput(++fd, size);
}	

#ifdef _MSWINDOWS_

#else

static FILE *fifo = NULL;
static int ctrl = -1;

static void ctrl_name(char *buf, const char *id, size_t size)
{
	if(*id == '/')
		++id;

	snprintf(buf, size, "/var/run/%s", id);

	if(cpr_isdir(buf))	
		snprintf(buf, size, "/var/run/%s/%s.ctrl", id, id);
	else
		snprintf(buf, size, "/tmp/.%s.ctrl", id);
}

void service::closectrl(void)
{
	if(ctrl > -1) {
		close(ctrl);
		ctrl = -1;
	}
}

bool service::openctrl(const char *id)
{
	char buf[65];

	ctrl_name(buf, id, sizeof(buf));
	ctrl = open(buf, O_RDWR);
	if(ctrl < 0)
		return false;
	return true;
}

bool service::control(const char *id, const char *fmt, ...)
{
	va_list args;
	int fd;
	char buf[512];
	char *ep;
	size_t len;

	va_start(args, fmt);

	if(id) {
		ctrl_name(buf, id, 65);
		fd = open(buf, O_RDWR);
		if(fd < 0)
			return false;
	}
	else if(ctrl > -1)
		fd = ctrl;
	else 
		return false;

	vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	va_end(args); 
	
	ep = strchr(buf, '\n');
	if(ep)
		*ep = 0;

	len = strlen(buf);
	buf[len++] = '\n';
		
	write(fd, buf, len);
	if(ctrl != fd)
		close(fd);

	return true;
}

size_t service::createctrl(const char *id)
{
	char buf[65];

	ctrl_name(buf, id, sizeof(buf));
	remove(buf);
	if(mkfifo(buf, 0660))
		return 0;

	fifo = fopen(buf, "r+");
	if(fifo)
		return 512;
	else
		return 0;
}

void service::releasectrl(const char *id)
{
	char buf[65];

	if(!fifo)
		return;

	ctrl_name(buf, id, sizeof(buf));

	fclose(fifo);
	fifo = NULL;
	remove(buf);
}

size_t service::recvctrl(char *buf, size_t max)
{
	char *ep;

	if(!fifo)
		return 0;

	fgets(buf, max, fifo);
	ep = strchr(buf, '\n');
	if(ep) {
		*ep = 0;
		return strlen(buf);
	}
	else
		return 0;
}

#endif

// vim: set ts=4 sw=4:

