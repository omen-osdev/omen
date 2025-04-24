#include <omen/managers/cpu/signal.h>
#include <omen/libraries/std/stdint.h>
#include <omen/apps/panic/panic.h>

int sigaction(process_t * task, int signum, struct sigaction * act, struct sigaction * oldact) {
    if (task == 0) panic("No such task\n");
    if (signal > NSIG || signal < 0) panic("Signal out of bounds\n");
    if (signal == SIGKILL) panic("SIGKILL cannot be caught\n");
    if (signal == SIGSTOP) panic("SIGSTOP cannot be caught\n");

    if (act == 0) {
        panic("No such act\n");
        return;
    } 
    if (oldact != 0) {
        oldact->sa_handler = task->signal_handlers[signal].sa_handler;
        oldact->sa_sigaction = task->signal_handlers[signal].sa_sigaction;
        oldact->sa_mask = task->signal_handlers[signal].sa_mask;
        oldact->sa_flags = task->signal_handlers[signal].sa_flags;
        oldact->sa_restorer = task->signal_handlers[signal].sa_restorer;
    }

    task->signal_handlers[signal].sa_handler = act->sa_handler;
    task->signal_handlers[signal].sa_sigaction = act->sa_sigaction;
    task->signal_handlers[signal].sa_mask = act->sa_mask;
    task->signal_handlers[signal].sa_flags = act->sa_flags;
    task->signal_handlers[signal].sa_restorer = act->sa_restorer;

    return 0;
}

int sigpending(process_t * task, sigset_t * set) {
    if (task == 0) {
        panic("No such task\n");
        return;
    }
    if (set == 0) {
        panic("No such set\n");
        return;
    }

    for (int i = 0; i < NSIG; i++) {
        if (task->signal_queue[i] != 0) {
            set[i / 8] |= (1 << (i % 8));
        }
    }
    return 0;
}

int sigprocmask(thread_t * thread, int how, const sigset_t * set, sigset_t * oldset) {

    if (thread == 0) {
        panic("No such thread\n");
        return;
    }
    if (set == 0) {
        panic("No such set\n");
        return;
    }

    if (oldset != 0) {
        *oldset = thread->sigprocmask;
    }

    switch (how) {
        case SIG_BLOCK:
            thread->sigprocmask |= *set;
            break;
        case SIG_UNBLOCK:
            thread->sigprocmask &= ~(*set);
            break;
        case SIG_SETMASK:
            thread->sigprocmask = *set;
            break;
        default:
            panic("Invalid how value\n");
            return;
    }

    return 0;
}

int sigsuspend(thread_t* thread, const sigset_t *mask) {
    if (thread == 0) {
        panic("No such thread\n");
        return;
    }
    if (mask == 0) {
        panic("No such mask\n");
        return;
    }

    thread->sigsuspend_mask = *mask;
    sched();
    return 0;
}

int kill(process_t * task, int signal) {
    if (task == 0) {
        panic("No such task\n");
        return;
    }
    if (signal > NSIG || signal < 0) {
        panic("Signal out of bounds\n");
        return;
    }

    thread_t * thread = get_current_thread();
    if (thread == 0) {
        panic("No current thread\n");
        return;
    }
    
    //Add signal to signal_queue linked list
    signal_t * signal_struct = kmalloc(sizeof(struct task_signal));
    signal_struct->signal = signal;
    signal_struct->next = 0;
    signal_struct->invoked_by_pid = thread->process->pid;
    signal_struct->invoked_by_thread = thread->id;

    if (task->signal_queue[signal] == 0) {
        task->signal_queue[signal] = signal_struct;
    } else {
        signal_t * current = task->signal_queue[signal];
        while (current->next != 0) {
            current = current->next;
        }
        current->next = signal_struct;
    }

    return 0;
}

int sigaltstack(thread_t * thread, struct stack* ss, struct stack * old_ss) {
    if (thread == 0) {
        panic("No such thread\n");
        return;
    }

    if (ss == 0) {
        panic("No such stack\n");
        return;
    }

    if (old_ss != 0) {
        old_ss->base = thread->altstack_base;
        old_ss->flags = thread->altstack_flags;
        old_ss->top = thread->altstack;
    }

    thread->altstack = ss->top;
    thread->altstack_base = ss->base;
    thread->altstack_flags = ss->flags;
    return 0;
}