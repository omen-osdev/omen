#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <omen/libraries/std/stdint.h>
#include <omen/libraries/allocators/heap_allocator.h>

#define NSIG		65
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

#define SIG_BLOCK    0
#define SIG_UNBLOCK  1
#define SIG_SETMASK  2

#define SIG_IGN     0
#define SIG_DFL     1
#define SIG_ERR     2
#define SIG_HOLD    3

typedef struct siginfo {
	int      si_signo;     /* Signal number */
	int      si_errno;     /* An errno value */
	int      si_code;      /* Signal code */
	int      si_trapno;    /* Trap number that caused
							  hardware-generated signal
							  (unused on most architectures) */
	int16_t    si_pid;       /* Sending process ID */
	int16_t    si_uid;       /* Real user ID of sending process */
	int      si_status;    /* Exit value or signal */
	uint64_t  si_utime;     /* User time consumed */
	uint64_t  si_stime;     /* System time consumed */
	int si_value;     /* Signal value */
	int      si_int;       /* POSIX.1b signal */
	void    *si_ptr;       /* POSIX.1b signal */
	int      si_overrun;   /* Timer overrun count;
							  POSIX.1b timers */
	int      si_timerid;   /* Timer ID; POSIX.1b timers */
	void    *si_addr;      /* Memory location which caused fault */
	long     si_band;      /* Band event (was int in
							  glibc 2.3.2 and earlier) */
	int      si_fd;        /* File descriptor */
	short    si_addr_lsb;  /* Least significant bit of address
							  (since Linux 2.6.32) */
	void    *si_lower;     /* Lower bound when address violation
							  occurred (since Linux 3.19) */
	void    *si_upper;     /* Upper bound when address violation
							  occurred (since Linux 3.19) */
	int      si_pkey;      /* Protection key on PTE that caused
							  fault (since Linux 4.6) */
	void    *si_call_addr; /* Address of system call instruction
							  (since Linux 3.5) */
	int      si_syscall;   /* Number of attempted system call
							  (since Linux 3.5) */
	unsigned int si_arch;  /* Architecture of attempted system call
							  (since Linux 3.5) */
} siginfo_t;

struct sigaction {
    void     (*sa_handler)(int);
    void     (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t   sa_mask;
    int        sa_flags;
    void     (*sa_restorer)(void);
};

typedef struct task_signal {
    int signo;
    struct task_signal *next;
} signal_t;

int dequeue_signal(struct task_signal **squeue, signal_t * signal, int signo);
void __attribute__((__section__(".vdso"))) signal_trampoline();

int sigaction(struct sigaction * shandlers, int signum, struct sigaction * act, struct sigaction * oldact);
int sigsuspend(sigset_t* sigsuspend_mask, const sigset_t *mask);
int sigprocmask(sigset_t * sigprocmask, int how, const sigset_t * set, sigset_t * oldset);
int sigpending(struct task_signal **squeue, sigset_t * set);
int kill(struct task_signal **squeue, int signal);
int sigaltstack(struct stack* stack, struct stack* ss, struct stack * old_ss);
#endif