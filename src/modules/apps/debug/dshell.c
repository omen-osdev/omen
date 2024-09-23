#include <omen/apps/debug/dshell.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/mem/vmm.h>
#include <dummy/dummy.h>
#include <ps2/ps2.h>
#include <generic/config.h>

char current_command[64];

void help(int argc, char* argv[]);

struct command {
    char keyword[32];
    void (*handler)(int argc, char* argv[]);
};

void test(int argc, char* argv[]) {
    if (argc < 1) {
        kprintf("Prints all the parameters\n");
        kprintf("Usage: test <param1> ...\n");
        return;
    }

    for (int i = 1; i < argc; i++) {
        kprintf("%s\n", argv[i]);
    }
}

void spawn(int argc, char* argv[]) {
    process_t * new = create_user_process(dummy_main);
    mprotect(new->vm, dummy_main, 0x1000, VMM_USER_BIT);
    kprintf("New process created with pid %d\n", new->pid);
    add_process(new);
    yield(new);
}

struct command cmdlist[] = {
    {
        .keyword = "test",
        .handler = test
    },
    {
        .keyword = "spawn",
        .handler = spawn
    },
    {
        .keyword = "help",
        .handler = help
    }
};

void help(int argc, char* argv[]) {
    if (argc < 2) {
        kprintf("Available commands:\n");
        for (long unsigned int i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
            kprintf("%s ", cmdlist[i].keyword);
            if (i > 1 && i % 5 == 0) {
                kprintf("\n");
            }
        }
        kprintf("\n");
    } else {
        //Search for command, print all matches, even if partial
        for (long unsigned int i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
            if (strstr(cmdlist[i].keyword, argv[1]) != 0) {
                kprintf("%s\n", cmdlist[i].keyword);
                if (i > 1 && i % 5 == 0) {
                    kprintf("\n");
                }
            }
        }
    }
}

void print_prompt() {
    kprintf("dev@omen:~$ ");
}

void ex_dbgshell(const char * command) {
    char cmd[1024] = {0};
    strncpy(cmd, command, strlen(command));
    char* args[32] = {0};
    int argc = 0;
    char* tok = strtok(cmd, " ");
    while (tok != 0) {
        args[argc] = tok;
        argc++;
        tok = strtok(0, " ");
    }

    for (uint32_t i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
        if (strcmp(cmdlist[i].keyword, args[0]) == 0) {
            cmdlist[i].handler(argc, args);
            break;
        }
    }

    print_prompt();
}

void dshell_cb(void* data, char c, int ignore) {
    (void)data;
    (void)ignore;
    //kprintf("%x", c);
    switch (c) {
        case 0x0:
            break;
        case 0x1b:
            if (strlen(current_command) < 63) {
                current_command[strlen(current_command)] = c;
            }
            #ifdef DSHELL_ECHO
                kprintf(" ");
            #endif
            break;
        case 0xd:
            #ifdef DSHELL_ECHO
                kprintf("\n");
            #endif
            if (strlen(current_command) > 0) {
                ex_dbgshell(current_command);
                memset(current_command, 0, 64);
            }
            break;
        case 0x8:
            if (strlen(current_command) > 0) {
                current_command[strlen(current_command) - 1] = 0;
            }
            #ifdef DSHELL_ECHO
                kprintf("\b \b");
            #endif
            break;
        default:
            if (strlen(current_command) < 63) {
                current_command[strlen(current_command)] = c;
            }
            #ifdef DSHELL_ECHO
                kprintf("%c", c);
            #endif
            break;
    }
}

void init_dshell() {
    struct ps2_kbd_ioctl_subscriptor subscriptor ={
        .parent = 0,
        .handler = dshell_cb
    };
    ps2_subscribe((void*)&subscriptor, PS2_DEVICE_KEYBOARD, PS2_DEVICE_GENERIC_EVENT);
    kprintf("Debug shell initialized\n");
    help(0, 0);
    print_prompt();
}