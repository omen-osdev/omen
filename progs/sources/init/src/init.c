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
    sys_munmap(file_buffer, file_stat.st_size);
    sys_close(fd);  
}

void write_file(const char * text) {
    int fd = sys_open("hdap2/data/lorem-ipsum.txt", 0);
    if (fd < 0) {
        printf("Failed to open file for writing\n");
        return;
    }

    struct stat file_stat;
    memset(&file_stat, 0, sizeof(struct stat));
    sys_fstat(fd, &file_stat);
    printf("File size: %d\n", file_stat.st_size);
    printf("File mode: %d\n", file_stat.st_mode);

    char * file_buffer = sys_mmap(NULL, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file_buffer == NULL) {
        printf("Failed to map file\n");
        sys_close(fd);
        return;
    }
    printf("File mapped at: %p\n", file_buffer);

    //Write to the file
    printf("Writing to file: %s\n", text);
    for (uint64_t i = 0; i < strlen(text); i++) {
        file_buffer[i] = text[i];
    }

    sys_munmap(file_buffer, file_stat.st_size);
    sys_close(fd);
}

void test_cow() {
    char * buffer = sys_mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == NULL) {
        printf("Failed to map memory\n");
        return;
    }

    volatile short pid = sys_fork();
    if (pid == 0) {
        printf("I am the child\n");
        buffer[0] = 'C';
        printf("Child buffer before: %s\n", buffer);
        sys_sched_yield();
        printf("Child buffer after: %s\n", buffer);
    } else {
        printf("I am the parent\n");
        buffer[0] = 'P';
        printf("Parent buffer before: %s\n", buffer);
        sys_sched_yield();
        printf("Parent buffer after: %s\n", buffer);
    }

    sys_munmap(buffer, 0x1000);
}

int main(int argc, char* argv[]) {
    printf("Hello from init!\n");
    volatile short pid = sys_fork();
    if (pid == 0) {
        printf("I am the child\n");
        print_file();
        write_file("Hello from the child process!\n");
        print_file();
    } else {
        sys_execve("hdap2/export/idle.elf", NULL, NULL);
    }

    test_cow();

    while (1) {
        //printf("Init process is running\n");
        sys_sched_yield();
    }
}