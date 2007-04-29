#include <config.h>
#include <ucommon/file.h>
#include <ucommon/process.h>

#if _POSIX_MAPPED_FILES > 0
#include <sys/mman.h>
#endif

#if HAVE_FTOK
#include <sys/ipc.h>
#include <sys/shm.h>

extern "C" {
	extern key_t cpr_createipc(const char *name);
	extern key_t cpr_accessipc(const char *name);
	extern void cpr_unlinkipc(const char *name);
};
#endif

#include <errno.h>
#include <string.h>

using namespace UCOMMON_NAMESPACE;

#if defined(_MSWINDOWS_)

MappedMemory::MappedMemory(const char *fn, size_t len)
{
	int share = FILE_SHARE_READ;
	int prot = FILE_MAP_READ;
	int page = PAGE_READONLY;
	int mode = GENERIC_READ;
	struct stat ino;
	char fpath[256];

	size = 0;
	used = 0;
	map = NULL;

	snprintf(fpath, sizeof(fpath), "c:/temp/%s.shm", fn + 1);
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

MappedMemory::~MappedMemory()
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
		key = cpr_createipc(name);
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
		key = cpr_accessipc(name);
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

MappedAssoc::MappedAssoc(mempager *pager, const char *fname, size_t size, unsigned isize) :
MappedMemory(fname, size), keyassoc(isize, pager)
{
	pthread_mutex_init(&mutex, NULL);
}

void *MappedAssoc::find(const char *id, size_t osize, size_t tsize)
{
	void *mem;
	
	pthread_mutex_lock(&mutex);
	mem = keyassoc::get(id); 
	if(mem) {
		pthread_mutex_unlock(&mutex);
		return mem;
	}
	
	mem = MappedMemory::sbrk(osize + tsize);
	cpr_strset((char *)mem, tsize, id);
	keyassoc::set((char *)mem, (caddr_t)(mem) + tsize);
	pthread_mutex_unlock(&mutex);
	return mem;
}

#if UCOMMON_ASYNC_IO > 0

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

void aio::cancel(void)
{
	if(!pending)
		return;

	count = 0;
	aio_cancel(cb.aio_fildes, &cb);
	err = aio_error(&cb);
	pending = false;
}

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

#endif

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

#if _POSIX_ASYNCHRONOUS_IO > 0

extern "C" bool cpr_isasync(fd_t fd)
{
	return fpathconf(fd, _POSIX_ASYNC_IO) != 0;
}

#else

extern "C" bool cpr_isasync(fd_t fd)
{
	return false;
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

#ifdef	HAVE_FTOK

static void ftok_name(const char *name, char *buf, size_t max)
{
	if(*name == '/')
		++name;

	if(cpr_isdir("/var/run/ipc"))
		snprintf(buf, sizeof(buf), "/var/run/ipc/%s", name);
	else
		snprintf(buf, sizeof(buf), "/tmp/.%s.ipc", name);
}

extern "C" void cpr_unlinkipc(const char *name)
{
	char buf[65];

	ftok_name(name, buf, sizeof(buf));
	remove(buf);
}

extern "C" key_t cpr_createipc(const char *name)
{
	char buf[65];
	int fd;

	ftok_name(name, buf, sizeof(buf));
	remove(buf);
	fd = creat(buf, 0660);
	close(fd);
	return ftok(buf, 'u');
}

extern "C" key_t cpr_accessipc(const char *name)
{
	char buf[65];

	ftok_name(name, buf, sizeof(buf));
	return ftok(buf, 'u');
}

#endif

