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

int main(int argc, char* argv[]) {
    printf("Hello from init!\n");
    volatile short pid = sys_fork();
    if (pid == 0) {
        printf("I am the child\n");
    } else {
        printf("I am the parent\n");
        sys_exit(0);
    }
    print_file();
    while(1);
}