#include <omen/apps/debug/dshell.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/stdint.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/managers/cpu/process.h>
#include <omen/managers/mem/vmm.h>
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
        DBG_INFO("Prints all the parameters\n");
        DBG_INFO("Usage: test <param1> ...\n");
        return;
    }

    for (int i = 1; i < argc; i++) {
        DBG_INFO("%s\n", argv[i]);
    }
}

void spawn(int argc, char* argv[]) {
    panic("Not implemented\n");
    //process_t * new = create_user_process(dummy_main);
    //DBG_INFO("New process created with pid %d\n", new->pid);
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
        DBG_INFO("Available commands:\n");
        for (long unsigned int i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
            DBG_INFO("%s ", cmdlist[i].keyword);
            if (i > 1 && i % 5 == 0) {
                DBG_INFO("\n");
            }
        }
        DBG_INFO("\n");
    } else {
        //Search for command, print all matches, even if partial
        for (long unsigned int i = 0; i < sizeof(cmdlist) / sizeof(struct command); i++) {
            if (strstr(cmdlist[i].keyword, argv[1]) != 0) {
                DBG_INFO("%s\n", cmdlist[i].keyword);
                if (i > 1 && i % 5 == 0) {
                    DBG_INFO("\n");
                }
            }
        }
    }
}

void print_prompt() {
    DBG_INFO("dev@omen:~$ ");
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
    //DBG_INFO("%x", c);
    switch (c) {
        case 0x0:
            break;
        case 0x1b:
            if (strlen(current_command) < 63) {
                current_command[strlen(current_command)] = c;
            }
            #ifdef DSHELL_ECHO
                DBG_INFO(" ");
            #endif
            break;
        case 0xd:
            #ifdef DSHELL_ECHO
                DBG_INFO("\n");
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
                DBG_INFO("\b \b");
            #endif
            break;
        default:
            if (strlen(current_command) < 63) {
                current_command[strlen(current_command)] = c;
            }
            #ifdef DSHELL_ECHO
                DBG_INFO("%c", c);
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
    DBG_INFO("Debug shell initialized\n");
    help(0, 0);
    print_prompt();
}