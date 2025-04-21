#ifndef _SIGNAL_STD_H
#define _SIGNAL_STD_H

#define NSIG		32
typedef unsigned int sigval_t;
typedef unsigned long sigset_t;

#define SA_NOCLDSTOP 		0x0
#define SA_NOCLDWAIT 		0x1
#define SA_NODEFER 			0x2
#define SA_ONSTACK 			0x4
#define SA_RESETHAND 		0x8
#define SA_RESTART 			0x10
#define SA_RESTORER 		0x20
#define SA_SIGINFO 			0x40
#define SA_UNSUPPORTED 		0x80
#define SA_EXPOSE_TAGBITS 	0x100

#define SI_USER				0x0
#define SI_KERNEL	    	0x1
#define SI_QUEUE			0x2
#define SI_TIMER			0x3
#define SI_MESGQ			0x4
#define SI_ASYNCIO      	0x5
#define SI_SIGIO 			0x6	
#define SI_TKILL 			0x7
//For sigill
#define ILL_ILLOPC      	0x8
#define ILL_ILLOP			0x9
#define ILL_ILLADDR 		0xA
#define ILL_ILLTRP      	0xB
#define ILL_PRVOPC 			0xC
#define ILL_PRVREG 			0xD
#define ILL_COPROC 			0xE
#define ILL_BADSTK 			0xF
//For sigfpe
#define FPE_INTDIV 			0x10
#define FPE_INTOVF 			0x11
#define FPE_FLTDIV 			0x12
#define FPE_FLTOVF 			0x13
#define FPE_FLTUND 			0x14
#define FPE_FLTRES 			0x15
#define FPE_FLTINV 			0x16
#define FPE_FLTSUB 			0x17
//For sigsegv
#define SEGV_MAPERR 		0x18
#define SEGV_ACCERR 		0x19
#define SEGV_BNDERR 		0x1A
#define SEGV_PKUERR 		0x1B
//For sigbus
#define BUS_ADRALN 			0x1C
#define BUS_ADRERR 			0x1D
#define BUS_OBJERR 			0x1E
#define BUS_MCEERR_AR 		0x1F
#define BUS_MCEERR_AO 		0x20
//For sigtrap
#define TRAP_BRKPT 			0x21
#define TRAP_TRACE 			0x22
#define TRAP_BRANCH 		0x23
#define TRAP_HWBKPT 		0x24
//For SIGCHLD
#define CLD_EXITED 			0x25
#define CLD_KILLED 			0x26
#define CLD_DUMPED 			0x27
#define CLD_TRAPPED 		0x28
#define CLD_STOPPED 		0x29
#define CLD_CONTINUED 		0x2A
//For SIGIO/SIGPOLL
#define POLL_IN 			0x2B
#define POLL_OUT 			0x2C
#define POLL_MSG 			0x2D
#define POLL_ERR 			0x2E
#define POLL_PRI 			0x2F
#define POL_HUP 			0x30
//For SIGSYS
#define SYS_SECCOMP 		0x31

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
	sigval_t si_value;     /* Signal value */
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

typedef void (*sighandler_t)(int);
typedef void (*sigaction_t)(int, siginfo_t *, void *);

struct sigaction {
	sighandler_t sa_handler;
	sigaction_t sa_sigaction;
	unsigned long sa_flags;
	sigset_t sa_mask;
	int sa_restorer; //Always 0
};

#endif