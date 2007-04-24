#include <config.h>
#include <ucommon/file.h>
#include <ucommon/process.h>

#if _POSIX_MAPPED_FILES > 0
#include <sys/mman.h>
#endif

#include <errno.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

#if defined(_MSWINDOWS_) && !defined(_POSIX_MAPPED_FILES)

MappedFile::MappedFile(const char *fn, size_t len, size_t paging)
{
	int share = FILE_SHARE_READ;
	int prot = FILE_MAP_READ;
	int page = PAGE_READONLY;
	int mode = GENERIC_READ;
	struct stat ino;
	HANDLE fd;

	page = paging;
	size = used = 0;
	map = NULL;

	if(len) {
		prot = FILE_MAP_WRITE;
		page = PAGE_READWRITE;
		mode |= GENERIC_WRITE;
		share |= FILE_SHARE_WRITE;
		remove(fn);
	}
	fd = CreateFile(fn, mode, share, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if(fd == INVALID_HANDLE_VALUE) 
		return;

	if(len) {
		SetFilePointer(fd, (LONG)len, 0l, FILE_BEGIN);
		SetEndOfFile(fd);
	}
	else {
		SetFilePointer(fd, 0l, 0l, FILE_END);
		len = GetFilePointer(fd);
	}
	hmap = CreateFileMapping(fd, NULL, page, 0, 0, fn + 1);
	if(hmap == INVALID_HANDLE_VALUE) {
		CloseHandle(fd);
		return;
	}
	map = (caddr_t)MapViewOfFile(map, prot, 0, 0, len);
	if(map) {
		size = len;
		VirtualLock(map, size);
	}
	CloseHandle(fd);
}

MappedFile::~MappedFile()
{
	if(map) {
		VirtualUnlock(map, size);
		UnmapViewOfFile(hmap);
		CloseHandle(hmap);
		map = NULL;
		hmap = INVALID_HANDLE_VALUE;
	}
}

#else

MappedView::MappedView(const char *fn)
{
	int prot = PROT_READ;
	struct stat ino;
	int fd;
	size_t len = 0;

	size = 0;
	fd = shm_open(fn, O_RDONLY, 0660);
	if(fd > -1) {
		fstat(fd, &ino);
		len = ino.st_size - sizeof(MappedLock);
	}

	if(fd < 0)
		return;
	
	map = (caddr_t)mmap(NULL, len, prot, MAP_SHARED, fd, sizeof(MappedFile));
	if(map != (caddr_t)MAP_FAILED) {
		size = len;
		mlock(map, size);
	}
	close(fd);
}

MappedView::~MappedView()
{
	if(size) {
		munlock(map, size);
		munmap(map, size);
		size = 0;
		map = (caddr_t)MAP_FAILED;
	}
}
	
MappedBuffer::MappedBuffer(const char *fn, size_t bufsize)
{
	int prot = PROT_READ | PROT_WRITE;
	struct stat ino;
	int fd;

	size = 0;
	if(bufsize) {
		size = bufsize + sizeof(control);
		shm_unlink(fn);
		fd = shm_open(fn, O_RDWR | O_CREAT, 0660);
		if(fd > -1)
			ftruncate(fd, size);
	}
	else {
		fd = shm_open(fn, O_RDWR, 0660);
		if(fd > -1) {
			fstat(fd, &ino);
			size = ino.st_size;
		}
	}
	if(fd < 0)
		return;

	map = (caddr_t)mmap(NULL, size, prot, MAP_SHARED, fd, 0);
	close(fd);
	if(map == (caddr_t)MAP_FAILED) {
		size = 0;
		return;
	}
	mlock(map, size);
	buffer = (control *)map;
	if(bufsize) {
		Conditional::mapped(buffer);
		buffer->head = buffer->tail = sizeof(control);
		buffer->count = 0;
	}
}

MappedBuffer::~MappedBuffer()
{
	if(size) {
		munlock(map, size);
		munmap(map, size);
		size = 0;
	}
}

unsigned MappedBuffer::getCount(size_t objsize)
{
	unsigned c;

	buffer->lock();
	c = buffer->count;
	buffer->unlock();
	return c;
}

void *MappedBuffer::get(void)
{
	void *dbuf;

	buffer->lock();
	while(!buffer->count)
		wait();
	dbuf = map + buffer->head;
	buffer->unlock();
	return dbuf;
}

void MappedBuffer::release(size_t objsize)
{
	buffer->lock();
	buffer->head += objsize;
	if(buffer->head >= size)
		buffer->head = sizeof(control);
	--buffer->count;
	buffer->signal();
	buffer->unlock();
}

void MappedBuffer::put(void *dbuf, size_t objsize)
{
	buffer->lock();
	while(buffer->head == buffer->tail && buffer->count)
		wait();
	memcpy(map + buffer->tail, dbuf, objsize);
	buffer->tail += objsize;
	if(buffer->tail >= size)
		buffer->tail = sizeof(control);
	++buffer->count;
	buffer->signal();
	buffer->unlock();
}

MappedFile::MappedFile(const char *fn, size_t len, size_t paging)
{
	int prot = PROT_READ | PROT_WRITE;
	struct stat ino;
	int fd;

	page = paging;
	size = 0;
	used = sizeof(MappedLock);
	lock = NULL;
	bool addlock = false;
	
	if(len) {
		addlock = true;
		len += sizeof(MappedLock);
//		prot |= PROT_WRITE;
		shm_unlink(fn);
		fd = shm_open(fn, O_RDWR | O_CREAT, 0660);
		if(fd > -1)
			ftruncate(fd, len);
	}
	else {
		fd = shm_open(fn, O_RDWR, 0660);
		if(fd > -1) {
			fstat(fd, &ino);
			len = ino.st_size;
		}
	}

	if(fd < 0)
		return;
	
	map = (caddr_t)mmap(NULL, len, prot, MAP_SHARED, fd, 0);
	if(map != (caddr_t)MAP_FAILED) {
		size = len;
		mlock(map, size);
	}
	if(addlock)
		new(map) MappedLock();
	lock = (MappedLock *)map;
	close(fd);
}

MappedFile::~MappedFile()
{
	if(size) {
		munlock(map, size);
		munmap(map, size);
		size = 0;
		map = (caddr_t)MAP_FAILED;
	}
}

#endif

void MappedFile::fault(void) 
{
	abort();
}

void *MappedFile::sbrk(size_t len)
{
	void *mp = (void *)(map + used);
	size_t old = size;
	if(used + len > size && !page)
		fault();
	if(used + len > size) {
#if defined(_MSWINDOWS_) && !defined(_POSIX_MAPPED_FILES)
		fault();
#else
		munlock(map, size);
		size += page;
		if(mremap(map, old, size, 0) != map)
			fault();
		mlock(map, size);
#endif
	}
	used += len;
	return mp;
}
	
void *MappedFile::get(size_t offset)
{
	offset += sizeof(MappedLock);

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
	
	mem = MappedFile::sbrk(osize + tsize);
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

void aio::write(fd_t fd, caddr_t buf, size_t len, off_t offset)
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

void aio::read(fd_t fd, caddr_t buf, size_t len, off_t offset)
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

void aio::write(fd_t fd, caddr_t buf, size_t len, off_t offset)
{
    ssize_t res = pwrite(fd, buf, len, offset);
    count = 0;
    err = 0;
    if(res < 1)
        err = errno;
    else
        count = res;
}

void aio::read(fd_t fd, caddr_t buf, size_t len, off_t offset)
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

aiopager::aiopager(fd_t fdes, unsigned pages, unsigned scan, off_t start, size_t ps)
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

#if _POSIX_MAPPED_FILES > 0

extern "C" caddr_t cpr_mapfile(const char *fn)
{
	struct stat ino;
	int fd = open(fn, O_RDONLY);
	void *map;

	if(fd < 0)
		return NULL;

	if(fstat(fd, &ino) || !ino.st_size) {
		close(fd);
		return NULL;
	}

	map = mmap(NULL, ino.st_size, PROT_READ, MAP_SHARED, fd, 0);
	msync(map, ino.st_size, MS_SYNC);
	close(fd);
	if(map == MAP_FAILED)
		return NULL;
	
	return (caddr_t)map;
}
#else

caddr_t cpr_mapfile(const char *fn)
{
	void *mem;
	struct stat ino;

	int fd = open(fn, O_RDONLY);
	if(fd < 0)
		return NULL;

	if(fstat(fd, &ino) || !ino.st_size) {
		close(fd);
		return NULL;
	}

	mem = malloc(ino.st_size);
	if(!mem)
		abort();
	
	read(fd, mem, ino.st_size);
	close(fd);
	return (caddr_t)mem;
}

#endif

