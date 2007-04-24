#include <config.h>
#include <ucommon/file.h>
#include <ucommon/process.h>

#if _POSIX_MAPPED_FILES > 0
#include <sys/mman.h>
#endif

#include <errno.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

#ifndef _MSWINDOWS_

#ifdef HAVE_PTHREAD_RWLOCKATTR_SETPSHARED

mapped_lock::mapped_lock()
{
	pthread_rwlockattr_t attr;

	pthread_rwlockattr_init(&attr);
	pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_rwlock_init(&control.lock, &attr);
	pthread_rwlockattr_destroy(&attr);
}

void mapped_lock::exclusive(void)
{
	pthread_rwlock_wrlock(&control.lock);
}

void mapped_lock::share(void)
{
	pthread_rwlock_rdlock(&control.lock);
}

void mapped_lock::release(void)
{
	pthread_rwlock_unlock(&control.lock);
}

#else

mapped_lock::mapped_lock()
{
	pthread_mutexattr_t attr;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	pthread_mutex_init(&control.mutex, &attr);
	pthread_mutexattr_destroy(&attr);
}

void mapped_lock::mapped_lock(void)
{
	pthread_mutex_lock(&control.mutex);
}

void mapped_lock::share(void)
{
	pthread_mutex_lock(&control.mutex);
}

void mapped_lock::release(void)
{
	pthread_mutex_unlock(&control.mutex);
}

#endif
#else
void mapped_lock::share(void)
{
}

void mapped_lock::release(void)
{
}

void mapped_lock::exclusive(void)
{
}
#endif

void mapped_lock::Exlock(void)
{
	exclusive();
}

void mapped_lock::Shlock(void)
{
	share();
}

void mapped_lock::Unlock(void)
{
	release();
}

#if defined(_MSWINDOWS_) && !defined(_POSIX_MAPPED_FILES)

MappedFile::MappedFile(const char *fn, size_t len, size_t paging)
{
	int share = FILE_SHARE_READ;
	int prot = FILE_MAP_READ;
	int page = PAGE_READONLY;
	int mode = GENERIC_READ;
	struct stat ino;
	char fpath[256];

	page = paging;
	size = used = 0;
	map = NULL;

	snprintf(fpath, sizeof(fpath), "c:/temp/mapped-%s", fn + 1);
	if(len) {
		prot = FILE_MAP_WRITE;
		page = PAGE_READWRITE;
		mode |= GENERIC_WRITE;
		share |= FILE_SHARE_WRITE;
		remove(fpath);
	}
	fd = CreateFile(fpath, mode, share, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if(fd == INVALID_HANDLE_VALUE) 
		return;

	if(len) {
		SetFilePointer(fd, (LONG)len, 0l, FILE_BEGIN);
		SetEndOfFile(fd);
	}
	else {
		len = SetFilePointer(fd, 0l, 0l, FILE_END);
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
}

MappedFile::~MappedFile()
{
	if(map) {
		VirtualUnlock(map, size);
		UnmapViewOfFile(hmap);
		CloseHandle(hmap);
		map = NULL;
		hmap = INVALID_HANDLE_VALUE;
		CloseHandle(fd);
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
		len = ino.st_size - sizeof(mapped_lock);
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

unsigned MappedBuffer::count(void)
{
	unsigned c;

	buffer->lock();
	c = buffer->count;
	buffer->unlock();
	return c;
}

void MappedBuffer::get(void *data, size_t objsize)
{
	buffer->lock();
	while(!buffer->count)
		wait();
	memcpy(data, map + buffer->head, objsize);
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
	used = sizeof(mapped_lock);
	lock = NULL;
	bool addlock = false;
	
	if(len) {
		addlock = true;
		len += sizeof(mapped_lock);
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
		new(map) mapped_lock();
	lock = (mapped_lock *)map;
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
	offset += sizeof(mapped_lock);

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
    ssize_t res = cpr_pwritefile(fd, buf, len, offset);
    count = 0;
    err = 0;
    if(res < 1)
        err = errno;
    else
        count = res;
}

void aio::read(fd_t fd, caddr_t buf, size_t len, off_t offset)
{
	ssize_t res = cpr_preadfile(fd, buf, len, offset);
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
	fd = cpr_closefile(fd);

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

#ifdef HAVE_PREAD

extern "C" ssize_t cpr_preadfile(fd_t fd, caddr_t data, size_t len, off_t offset)
{
	return pread(fd, data, len, offset);
}

extern "C" ssize_t cpr_pwritefile(fd_t fd, caddr_t data, size_t len, off_t offset)
{
	return pwrite(fd, data, len, offset);
}

#elif defined(_MSWINDOWS_)

extern "C" ssize_t cpr_preadfile(fd_t fd, caddr_t data, size_t len, off_t offset)
{
	DWORD count = (DWORD)-1;
	ENTER_EXCLUSIVE
	SetFilePointer(fd, (DWORD)len, NULL, FILE_BEGIN);
	ReadFile(fd, data, (DWORD)len, &count, NULL);
	EXIT_EXCLUSIVE
	return count;
}

extern "C" ssize_t cpr_pwritefile(fd_t fd, caddr_t data, size_t len, off_t offset)
{
	DWORD count = (DWORD)-1;
	ENTER_EXCLUSIVE
	SetFilePointer(fd, (DWORD)len, NULL, FILE_BEGIN);
	WriteFile(fd, data, (DWORD)len, &count, NULL);
	EXIT_EXCLUSIVE
	return count;
}

#else

extern "C" ssize_t cpr_preadfile(fd_t fd, caddr_t data, size_t len, off_t offset)
{
	ssize_t rtn;
	ENTER_EXCLUSIVE
	lseek(fd, offset, SEEK_SET);
	rtn = read(fd, data, len);
	EXIT_EXCLUSIVE
	return rtn;
}

extern "C" ssize_t cpr_pwritefile(fd_t fd, caddr_t data, size_t len, off_t offset)
{
	ssize_t rtn;
	ENTER_EXCLUSIVE
	lseek(fd, offset, SEEK_SET);
	rtn = write(fd, data, len);
	EXIT_EXCLUSIVE
	return rtn;
}

#endif

#ifdef _MSWINDOWS_

extern "C" bool cpr_isopen(fd_t fd)
{
	if(fd == INVALID_HANDLE_VALUE)
		return false;
	return true;
}

extern "C" void cpr_seekfile(fd_t fd, off_t offset)
{
	SetFilePointer(fd, offset, NULL, FILE_BEGIN);
}

extern "C" size_t cpr_filesize(fd_t fd)
{
	DWORD pos, end;
	pos = SetFilePointer(fd, 0l, NULL, FILE_CURRENT);
	end = SetFilePointer(fd, 0l, NULL, FILE_END);
	SetFilePointer(fd, pos, NULL, FILE_BEGIN);
	return (size_t)end;
}

extern "C" ssize_t cpr_readfile(fd_t fd, caddr_t data, size_t len)
{
	DWORD count = (DWORD)-1;
	ReadFile(fd, data, (DWORD)len, &count, NULL);
	return count;
}

extern "C" ssize_t cpr_writefile(fd_t fd, caddr_t data, size_t len)
{
	DWORD count = (DWORD)-1;
	WriteFile(fd, data, (DWORD)len, &count, NULL);
	return count;
}

extern "C" fd_t cpr_closefile(fd_t fd)
{
	if(fd != INVALID_HANDLE_VALUE)
		CloseHandle(fd);
	return INVALID_HANDLE_VALUE;
}

#else

extern "C" bool cpr_isopen(fd_t fd)
{
	if(fd > -1)
		return true;

	return false;
}

extern "C" void cpr_seekfile(fd_t fd, off_t offset)
{
	lseek(fd, offset, SEEK_SET);
}

extern "C" size_t cpr_filesize(fd_t fd)
{
	struct stat ino;

	if(fd < 0 || fstat(fd, &ino))
		return 0;

	return ino.st_size;
}

extern "C" ssize_t cpr_readfile(fd_t fd, caddr_t data, size_t len)
{
	return read(fd, data, len);
}

extern "C" ssize_t cpr_writefile(fd_t fd, caddr_t data, size_t len)
{
	return write(fd, data, len);
}

extern "C" fd_t cpr_closefile(fd_t fd)
{
	if(fd > -1)
		close(fd);
	return -1;
}

extern "C" fd_t cpr_openfile(const char *fn, bool rewrite)
{
	if(rewrite)
		return open(fn, O_RDWR);
	else
		return open(fn, O_RDONLY);
}

#endif

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
	cpr_closefile(fd);
	if(map == MAP_FAILED)
		return NULL;
	
	return (caddr_t)map;
}
#else

caddr_t cpr_mapfile(const char *fn)
{
	caddr_t mem;
	fd_t fd;
	size_t size;

	fd = cpr_openfile(fn, false);
	if(!cpr_isopen(fd))
		return NULL;

	size = cpr_filesize(fd);
	if(size < 1) {
		cpr_closefile(fd);
		return NULL;
	}
	mem = (caddr_t)malloc(size);
	if(!mem)
		abort();
	
	cpr_readfile(fd, mem, size);
	cpr_closefile(fd);
	return (caddr_t)mem;
}

#endif

