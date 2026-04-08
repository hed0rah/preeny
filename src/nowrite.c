#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdarg.h>

#include "logging.h"

// write-intent flags to strip
#define WRITE_FLAGS (O_WRONLY|O_RDWR|O_CREAT|O_EXCL|O_TRUNC|O_APPEND)

int (*original_open)(const char *pathname, int flags, ...);
int (*original_openat)(int dirfd, const char *pathname, int flags, ...);

__attribute__((constructor)) void preeny_nowrite()
{
	original_open = dlsym(RTLD_NEXT, "open");
	original_openat = dlsym(RTLD_NEXT, "openat");
}

static int sanitize_flags(int flags)
{
	// only strip flags that imply write intent
	// preserve O_CLOEXEC, O_DIRECTORY, O_NOFOLLOW, O_NOCTTY, O_NONBLOCK, etc.
	flags &= ~WRITE_FLAGS;
	flags |= O_RDONLY;
	return flags;
}

int open(const char *pathname, int flags, ...) {
	int clean = sanitize_flags(flags);
	preeny_info("nowrite: open(\"%s\", %#x -> %#x)\n", pathname, flags, clean);
	return original_open(pathname, clean);
}

int openat(int dirfd, const char *pathname, int flags, ...) {
	int clean = sanitize_flags(flags);
	preeny_info("nowrite: openat(%d, \"%s\", %#x -> %#x)\n", dirfd, pathname, flags, clean);
	return original_openat(dirfd, pathname, clean);
}

