#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <omen/managers/cpu/process.h>
#include <omen/libraries/std/stdint.h>

#define NSIG		32
typedef unsigned long sigset_t;

#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO

#define SIGPWR		30
#define SIGSYS		31
#define	SIGUNUSED	31
#define SIGRTMIN	32
#define SIGRTMAX	_NSIG

#define SA_RESTORER	0x04000000

#define MINSIGSTKSZ	2048
#define SIGSTKSZ	8192

#define SIG_BLOCK    1
#define SIG_UNBLOCK  2
#define SIG_SETMASK  3

struct sigaction {
    void     (*sa_handler)(int);
    void     (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t   sa_mask;
    int        sa_flags;
    void     (*sa_restorer)(void);
};

typedef struct task_signal {
    int signo;
    int invoked_by_pid;
    int invoked_by_thread;
    struct task_signal *next;
} signal_t;

int sigaction(process_t * task, int signum, struct sigaction * act, struct sigaction * oldact);
int sigsuspend(thread_t* thread, const sigset_t *mask);
int sigprocmask(thread_t* thread, int how, const sigset_t * set, sigset_t * oldset);
int sigpending(process_t * task, sigset_t * set);
int kill(process_t * task, int signal);
int sigaltstack(thread_t* thread, struct stack* ss, struct stack * old_ss);
    #endif