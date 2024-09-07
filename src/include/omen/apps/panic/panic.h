//This is only for when we port CrowdStrike to our project

#ifndef _PANIC_H
#define _PANIC_H

__attribute__((noreturn)) void panic(const char * message);

#endif