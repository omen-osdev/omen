#include <minilibc.h>

int strlen(const char* str) {
    int i = 0;
    while (str[i] != 0) {
        i++;
    }
    return i;
}

#define PROMPT "root@omen:/$ \n"

int main(int argc, char* argv[]) {
    char text[32] = "I am the ";
    int j = 0;
    while (text[j] != 0) {
        j++;
    }
    
    volatile short pid = sys_fork();
    if (pid == 0) {
        const char child[] = "child\n";
        
        int i = 0;
        while (child[i] != 0) {
            text[j + i] = child[i];
            i++;
        }

        sys_write(1, text, 32);

    } else {
        const char parent[] = "parent\n";

        int i = 0;
        while (parent[i] != 0) {
            text[j + i] = parent[i];
            i++;
        }

        sys_write(1, text, 32);
        sys_exit(0);
    }

    char command_buffer[128];
    while(1) {
        sys_write(1, PROMPT, strlen(PROMPT));

        char c;
        int i = 0;
        sys_read(0, &c, 1);
        while (c != '\n' && i < 128) {
            command_buffer[i] = c;
            i++;
            sys_read(0, &c, 1);
        }
        
        command_buffer[i] = 0;
        sys_write(1, command_buffer, strlen(command_buffer));
        sys_write(1, "\n", 1);
    }
}