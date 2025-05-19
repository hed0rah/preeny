#define _GNU_SOURCE

#include <bits/types/sigset_t.h>
#include <bits/types/struct_timeval.h>
#include <dlfcn.h>
#include <sys/select.h>
#include <time.h>

int (*real_pselect)(int, fd_set*, fd_set*, fd_set*, const struct timespec*, const sigset_t*);
int (*real_pselect6)(int, fd_set*, fd_set*, fd_set*, const struct timespec*, const sigset_t*);
int (*real_select)(int, fd_set*, fd_set*, fd_set*, struct timeval*);

__attribute__((constructor)) void __depselect_init(void)
{
    real_pselect = dlsym(RTLD_NEXT, "pselect");
    real_pselect6 = dlsym(RTLD_NEXT, "pselect6");
    real_select = dlsym(RTLD_NEXT, "select");
}

int pselect(int nfds, fd_set* restrict readfds, fd_set* restrict writefds, fd_set* restrict exceptfds, const struct timespec* restrict timeout, const __sigset_t* restrict sigmask)
{
    return 1;
}

int pselect6(int nfds, fd_set* restrict readfds, fd_set* restrict writefds, fd_set* restrict exceptfds, const struct timespec* restrict timeout, const __sigset_t* restrict sigmask)
{
    return 1;
}

int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict exceptfds, struct timeval *restrict timeout)
{
    return 1;
}