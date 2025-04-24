#include <omen/managers/cpu/signal.h>
#include <omen/libraries/std/stdint.h>
#include <omen/apps/panic/panic.h>

void __attribute__((__section__(".vdso"))) signal_trampoline() {

}

int sigaction(struct sigaction * shandlers, int signum, struct sigaction * act, struct sigaction * oldact) {
    if (shandlers == 0) panic("Invalid sigaction structs\n");
    if (signum > NSIG || signum < 0) panic("Signal out of bounds\n");
    if (signum == SIGKILL) panic("SIGKILL cannot be caught\n");
    if (signum == SIGSTOP) panic("SIGSTOP cannot be caught\n");

    if (act == 0) {
        panic("No such act\n");
        return -1;
    } 

    if (oldact != 0) {
        oldact->sa_handler = shandlers[signum].sa_handler;
        oldact->sa_sigaction = shandlers[signum].sa_sigaction;
        oldact->sa_mask = shandlers[signum].sa_mask;
        oldact->sa_flags = shandlers[signum].sa_flags;
        oldact->sa_restorer = shandlers[signum].sa_restorer;
    }

    shandlers[signum].sa_handler = act->sa_handler;
    shandlers[signum].sa_sigaction = act->sa_sigaction;
    shandlers[signum].sa_mask = act->sa_mask;
    shandlers[signum].sa_flags = act->sa_flags;
    shandlers[signum].sa_restorer = act->sa_restorer;

    return 0;
}

int sigpending(struct task_signal **squeue, sigset_t * set) {
    if (squeue == 0) {
        panic("No such squeue\n");
        return -1;
    }
    if (set == 0) {
        panic("No such set\n");
        return -1;
    }

    for (int i = 0; i < NSIG; i++) {
        if (squeue[i] != 0) {
            set[i / 8] |= (1 << (i % 8));
        }
    }
    return 0;
}

int sigprocmask(sigset_t * sigprocmask, int how, const sigset_t * set, sigset_t * oldset) {

    if (sigprocmask == 0) {
        panic("No such sigprocmask\n");
        return -1;
    }
    if (set == 0) {
        panic("No such set\n");
        return -1;
    }

    if (oldset != 0) {
        *oldset = *sigprocmask;
    }

    switch (how) {
        case SIG_BLOCK:
            *sigprocmask |= *set;
            break;
        case SIG_UNBLOCK:
            *sigprocmask &= ~(*set);
            break;
        case SIG_SETMASK:
            *sigprocmask = *set;
            break;
        default:
            panic("Invalid how value\n");
            return -1;
    }

    return 0;
}

int sigsuspend(sigset_t* sigsuspend_mask, const sigset_t *mask) {
    if (sigsuspend_mask == 0) {
        panic("No such sigsuspend_mask\n");
        return -1;
    }
    if (mask == 0) {
        panic("No such mask\n");
        return -1;
    }

    *sigsuspend_mask = *mask;
    return 0;
}

int kill(struct task_signal **squeue, int signal) {
    if (squeue == 0) {
        panic("No such squeue\n");
        return -1;
    }
    if (signal > NSIG || signal < 0) {
        panic("Signal out of bounds\n");
        return -1;
    }
    
    //Add signal to signal_queue linked list
    signal_t * signal_struct = kmalloc(sizeof(struct task_signal));
    signal_struct->signo = signal;
    signal_struct->next = 0;

    if (squeue[signal] == 0) {
        squeue[signal] = signal_struct;
    } else {
        signal_t * current = squeue[signal];
        while (current->next != 0) {
            current = current->next;
        }
        current->next = signal_struct;
    }

    return 0;
}

int dequeue_signal(struct task_signal **squeue, signal_t * signal, int signo) {
    if (squeue == 0) {
        panic("No such squeue\n");
        return -1;
    }
    if (signo > NSIG || signo < 0) {
        panic("Signal out of bounds\n");
        return -1;
    }

    signal_t * signal_struct = squeue[signo];
    if (signal_struct != 0) {
        //Dequeue signal from signal_queue linked list, copy it to signal and free it
        squeue[signo] = signal_struct->next;
        signal_struct->next = 0;
        signal->signo = signal_struct->signo;
        signal->next = 0;
        kfree(signal_struct);
        return 1;
    }
    return 0;
}

int sigaltstack(struct stack* stack, struct stack* ss, struct stack * old_ss) {
    if (stack == 0) {
        panic("No such dest stack\n");
        return -1;
    }

    if (ss == 0) {
        panic("No such stack\n");
        return -1;
    }

    if (old_ss != 0) {
        old_ss->base = stack->base;
        old_ss->flags = stack->flags;
        old_ss->top = stack->top;
    }

    stack->top = ss->top;
    stack->base = ss->base;
    stack->flags = ss->flags;
    return 0;
}