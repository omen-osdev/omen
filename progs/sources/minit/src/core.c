#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <linux/fb.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
void tty_test() {
    int fd = open("/dev/tty", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/tty");
        exit(EXIT_FAILURE);
    }

    // Set the terminal to raw mode
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        perror("Failed to get terminal attributes");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    cfmakeraw(&tty);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Failed to set terminal attributes");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Write a test message
    const char* message = "Hello from tty_test!\n";
    write(fd, message, strlen(message));

    close(fd);
}

int main(int argc, char* argv[]){
    setenv("HOME", "/usr", 1);
    setenv("PWD", getenv("HOME"), 1);
    setenv("PATH", "/usr/bin", 1);
    setenv("PATHSTORE", "/usr/store", 1);
    setenv("USER", "root", 1);
    setenv("HOSTNAME", "omen", 1);
    setenv("TERM", "gnome-256color", 1);

    chdir(getenv("HOME"));
    setenv("PS1", "\\[\e[0;92m$USER@$HOSTNAME\e[0;37m: \e[0;94m\\w\e[0;37m\\$\e[0m\\] ", 1); // '#' is used to indicate root user session

    tty_test();

    char* exe_argv[2] = {"/usr/bin/bash", NULL};
    execvp("/usr/bin/bash", exe_argv);

    perror("init: /usr/bin/bash not found");
    return EXIT_FAILURE;
}