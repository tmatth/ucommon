#include <config.h>
#include <ucommon/file.h>
#include <ucommon/process.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

MappedFile::MappedFile(const char *fn, size_t len)
{
	int prot = PROT_READ;
	struct stat ino;

	size = used = 0;
	if(len) {
		prot |= PROT_WRITE;
		remove(fn);
		fd = creat(fn, 0660);
		if(fd > -1)
			lseek(fd, len, SEEK_SET);
	}
	else {
		fd = open(fn, O_RDONLY);
		if(fd > -1) {
			fstat(fd, &ino);
			len = ino.st_size;
		}
	}

	if(fd < 0)
		return;
	
	map = (caddr_t)mmap(NULL, len, prot, MAP_SHARED, fd, 0);
	if(map == (caddr_t)MAP_FAILED)
		close(fd);
	else
		size = len;
}

MappedFile::~MappedFile()
{
	if(size) {
		munmap(map, size);
		close(fd);
		size = 0;
		map = (caddr_t)MAP_FAILED;
	}
}

void MappedFile::fault(void) 
{
	abort();
}

void MappedFile::sync(void)
{
	if(!size)
		return;

	msync(map, size, MS_ASYNC);
}

void *MappedFile::brk(size_t len)
{
	void *mp = (void *)(map + used);
	if(used + len > size)
		fault();
	used += len;
	return mp;
}
	
void *MappedFile::get(size_t offset)
{
	if(offset >= size)
		fault();
	return (void *)(map + offset);
}

MappedAssoc::MappedAssoc(mempager *pager, const char *fname, size_t size, unsigned isize) :
MappedFile(fname, size), keyassoc(isize, pager)
{
}

void *MappedAssoc::find(const char *id, size_t osize, size_t tsize)
{
	void *mem = keyassoc::get(id);
	if(mem)
		return mem;
	
	mem = MappedFile::brk(osize + tsize);
	cpr_strset((char *)mem, tsize, id);
	keyassoc::set((char *)mem, (caddr_t)(mem) + tsize);
	return mem;
}

aio::aio()
{
	pending = false;
	count = 0;
	err = 0;
}

aio::~aio()
{
	cancel();
}

#ifdef	_POSIX_ASYNC_IO
void aio::cancel(void)
{
	if(!pending)
		return;

	count = 0;
	aio_cancel(cb.aio_fildes, &cb);
	err = aio_error(&cb);
	pending = false;
}
#else
void aio::cancel(void)
{
	count = 0;
#ifdef	ECANCELED
	err = ECANCELED;
#else
	err = -1;
#endif
}
#endif

#ifdef	_POSIX_ASYNC_IO

bool aio::isPending(void)
{
	if(!pending)
		return false;
	if(err == EINPROGRESS)
		err = aio_error(&cb);
	if(err != EINPROGRESS)
		return false;
	return true;
}

ssize_t aio::result(void)
{
	ssize_t res;
	if(err == EINPROGRESS)
		err = aio_error(&cb);
	if(err == EINPROGRESS)
		return 0;

	pending = false;
	res = aio_return(&cb);
	if(res > -1)
		count = res;
	return res;
}	

void aio::write(int fd, caddr_t buf, size_t len, off_t offset)
{
    cancel();
	err = EINPROGRESS;
    memset(&cb, 0, sizeof(cb));
    cb.aio_fildes = fd;
    cb.aio_buf = buf;
    cb.aio_nbytes = len;
    cb.aio_offset = offset;
    if(!aio_write(&cb)) {
		count = 0;
		pending = true;
	}
}

void aio::read(int fd, caddr_t buf, size_t len, off_t offset)
{
	cancel();
	err = EINPROGRESS;
	memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = fd;
	cb.aio_buf = buf;
	cb.aio_nbytes = len;
	cb.aio_offset = offset;
	if(!aio_read(&cb)) {
		count = 0;
		pending = true;
	}
}
		
#else
bool aio::isPending(void)
{
	return false;
}

ssize_t aio::result(void)
{
	if(!err)
		return (ssize_t)count;
	else
		return -1;
}

void aio::write(int fd, caddr_t buf, size_t len, off_t offset)
{
    ssize_t res = pwrite(fd, buf, len, offset);
    count = 0;
    err = 0;
    if(res < 1)
        err = errno;
    else
        count = res;
}

void aio::read(int fd, caddr_t buf, size_t len, off_t offset)
{
	ssize_t res = pread(fd, buf, len, offset);
	count = 0;
	err = 0;
	if(res < 1)
		err = errno;
	else
		count = res;
}
#endif

aiopager::aiopager(int fdes, unsigned pages, unsigned scan, off_t start, size_t ps)
{
	offset = start;
	count = pages;
	ahead = scan;
	pagesize = ps;
	written = count;
	fetched = 0;
	error = 0;
	fd = fdes;
	control = new aio[count];
	buffer = new char[count * pagesize];

	if(ahead > count)
		ahead = count;
}

aiopager::~aiopager()
{
	cancel();
	if(fd > -1)
		::close(fd);
	fd = -1;
	if(buffer)
		delete[] buffer;
	if(control)
		delete[] control;
	buffer = NULL;
	control = NULL;
}

void aiopager::sync(void)
{
	if(fd < 0)
		return;

	for(unsigned pos = 0; pos < count; ++pos) {
		while(control[pos].isPending())
			cpr_sleep(10);
		control[pos].result();
	}
	written = count;
	error = 0;
}

void aiopager::cancel(void)
{
	if(fd < 0)
		return;

	for(unsigned pos = 0; pos < count; ++pos) {
		if(control[pos].isPending())
			control[pos].cancel();
	}
	written = count;
	fetched = 0;
	error = 0;
}

caddr_t aiopager::buf(void)
{
	unsigned apos = index + fetched;
	off_t aoffset = offset + (pagesize * fetched);

	while(!error && ahead && fetched < ahead) {
		++fetched;
		if(apos >= count)
			apos -= count;
		control[apos].read(fd, &buffer[apos * pagesize], pagesize, aoffset);
		++apos;
		aoffset += pagesize;
	}
	if(!error || index != written) {
		if(control[index].isPending())
			return NULL;

		if(ahead || index == written)
			error = control[index].result();

		if(!ahead && ++written >= count)
			written = 0;
	}	
	return &buffer[index * pagesize];
}

void aiopager::next(size_t len)
{
	if(!len)
		len = pagesize;

	if(!ahead) {
		if(written == count)
			written = index;
		control[index].write(fd, &buffer[index * pagesize], len, offset);
	}
	if(++index >= count)
		index = 0;
	
	offset += len;

	if(fetched)
		--fetched;
}

size_t aiopager::len(void)
{
	if(error || control[index].isPending())
		return 0;
	
	return control[index].transfer();
}

extern "C" bool cpr_isdir(const char *fn)
{
	struct stat ino;

	if(stat(fn, &ino))
		return false;

	if(S_ISDIR(ino.st_mode))
		return true;

	return false;
}

extern "C" bool cpr_isfile(const char *fn)
{
	struct stat ino;

	if(stat(fn, &ino))
		return false;

	if(S_ISREG(ino.st_mode))
		return true;

	return false;
}


