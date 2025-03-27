#include <minilibc.h>
#define PROMPT "root@omen:/$ \n"

int strlen(const char* str) {
    int i = 0;
    while (str[i] != 0) {
        i++;
    }
    return i;
}

void* memset(void* ptr, int value, int num) {
    char* p = (char*)ptr;
    for (int i = 0; i < num; i++) {
        p[i] = value;
    }
    return ptr;
}

char invert_case(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    } else if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

void loop() {
    char command_buffer[128];
    char c = 0;
    int i = 0;
    while (1) {
        sys_write(1, PROMPT, strlen(PROMPT));
        sys_read(0, &c, 1);
        command_buffer[i++] = c;
        while (c != '\n' && i < 128) {
            sys_read(0, &c, 1);
            command_buffer[i++] = c;
        }

        command_buffer[i] = 0;
        i = 0;
        c = 0;

        sys_write(1, command_buffer, strlen(command_buffer));
        sys_write(1, "\n", 1);
    }
}

int main(int argc, char* argv[]) {
    
    volatile short pid = sys_fork();
    if (pid == 0) {
        sys_write(1, "I am the child\n", strlen("I am the child\n"));

    } else {
        sys_write(1, "I am the parent\n", strlen("I am the parent\n"));
        sys_exit(0);
    }
    
    loop();
}