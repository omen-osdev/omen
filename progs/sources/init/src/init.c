#include <minilibc.h>
#include <stdio.h>
#include <string.h>

void print_file() {
    int fd = sys_open("hdap2/data/lorem-ipsum.txt", 0);
    if (fd < 0) {
        printf("Failed to open file\n");
        return;
    }
    struct stat file_stat;
    memset(&file_stat, 0, sizeof(struct stat));
    sys_fstat(fd, &file_stat);
    printf("File size: %d\n", file_stat.st_size);
    printf("File mode: %d\n", file_stat.st_mode);

    char * file_buffer = sys_mmap(NULL, file_stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (file_buffer == NULL) {
        printf("Failed to map file\n");
        sys_close(fd);
        return;
    }
    printf("File mapped at: %p\n", file_buffer);

    printf("File content:\n");
    for (int i = 0; i < 0x40; i++) {
        if (file_buffer[i] == '\n') {
            printf("\n");
        } else {
            printf("%c", file_buffer[i]);
        }
    }
    printf("\n");
    sys_close(fd);  
}

void test_fork() {
    volatile short pid = sys_fork();
    if (pid == 0) {
        printf("I am the child\n");
    } else {
        printf("I am the parent\n");
        sys_exit(0);
    }
}

void test_cow() {
    char * buffer = sys_mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (buffer == NULL) {
        printf("Failed to map memory\n");
        return;
    }

    volatile short pid = sys_fork();
    if (pid == 0) {
        printf("I am the child\n");
        printf("Child buffer before: %s\n", buffer);
        buffer[0] = 'C';
        printf("Child buffer after: %s\n", buffer);
        sys_sched_yield();
    } else {
        printf("I am the parent\n");
        buffer[0] = 'P';
        printf("Parent buffer before: %s\n", buffer);
        sys_sched_yield();
        printf("Parent buffer after: %s\n", buffer);
    }
}

void test_sched() {
    volatile short pid = sys_fork();
    if (pid == 0) {
        printf("I am the child\n");
        while (1) {
            printf("Child is running\n");
            sys_sched_yield();
        }
    } else {
        printf("I am the parent\n");
        while (1) {
            printf("Parent is running\n");
            sys_sched_yield();
        }
    }
}

int main(int argc, char* argv[]) {
    printf("Hello from init!\n");
    //test_fork();
    test_sched();
    print_file();
    test_cow();
    while(1);
}