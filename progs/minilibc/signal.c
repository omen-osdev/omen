#include <signal.h>
#include <stddef.h>
#include <stdint.h>

int sigemptyset(sigset_t *set) {
    if (set == NULL) {
        return -1;
    }
    *set = 0;
    return 0;
}

int sigfillset(sigset_t *set) {
    if (set == NULL) {
        return -1;
    }
    *set = ~0;
    return 0;
}

int sigaddset(sigset_t *set, int signo) {
    if (set == NULL || signo < 1 || signo >= NSIG) {
        return -1;
    }
    *set |= (1 << signo);
    return 0;
}

int sigdelset(sigset_t *set, int signo) {
    if (set == NULL || signo < 1 || signo >= NSIG) {
        return -1;
    }
    *set &= ~(1 << signo);
    return 0;
}

int sigismember(sigset_t *set, int signo) {
    if (set == NULL || signo < 1 || signo >= NSIG) {
        return -1;
    }
    return (*set & (1 << signo)) != 0;
}