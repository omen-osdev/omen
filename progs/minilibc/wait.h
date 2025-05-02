#ifndef _WAIT_H
#define _WAIT_H
#define _WSTATUS(x)                         ((x) & 0177)
#define _WSTOPPED                           0177
#define _WCONTINUED                         0177777
#define WIFSTOPPED(x)                       (((x) & 0xff) == _WSTOPPED)
#define WSTOPSIG(x)                         (int)(((unsigned)(x) >> 8) & 0xff)
#define WIFSIGNALED(x)                      (_WSTATUS(x) != _WSTOPPED && _WSTATUS(x) != 0)
#define WTERMSIG(x)                         (_WSTATUS(x))
#define WIFEXITED(x)                        (_WSTATUS(x) == 0)
#define WEXITSTATUS(x)                      (int)(((unsigned)(x) >> 8) & 0xff)
#define WIFCONTINUED(x)                     (((x) & _WCONTINUED) == _WCONTINUED)
#define WCOREFLAG                           0200
#define WCOREDUMP(x)                        ((x) & WCOREFLAG)
#define W_EXITCODE(ret, sig)                ((ret) << 8 | (sig))
#define W_STOPCODE(sig)                     ((sig) << 8 | _WSTOPPED)

#define WREASON_EXIT                        0x01
#define WREASON_STOP                        0x02
#define WREASON_CONT                        0x04
#define WREASON_SIGNAL                      0x08

#define WNOHANG                             0x01
#define WUNTRACED                           0x02
#define WCONTINUED                          0x08
#define WEXITED                             0x04
#define WSTOPPED                            WUNTRACED
#define WNOWAIT                             0x10
#define WTRAPPED                            0x20

#define WAIT_ANY                            (-1)
#define WAIT_MYGRP                          0
#endif