#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
int main() {
    int fd = open("/home/norte/omen/sysroot/usr/lib/libc.so", O_RDONLY);
    uint8_t * buffer = malloc(64);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    ssize_t bytes_read = read(fd, buffer, 64);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    //Print the entire buffer
    printf("br: %d Buffer 1: \n", bytes_read);
    for (int i = 0; i < bytes_read; i++) {
        printf("%x ", buffer[i]);
    }
    printf("\n");

    //seek 64 bytes from the beginning
    off_t offset = lseek(fd, 64, SEEK_SET);
    if (offset < 0) {
        perror("lseek");
        close(fd);
        return 1;
    }

    //read 336 bytes
    uint8_t * buffer2 = malloc(336);
    bytes_read = read(fd, buffer2, 336);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    printf("\n");
    //Print the entire buffer 2
    printf("br2: %d Buffer 2: \n", bytes_read);
    for (int i = 0; i < bytes_read; i++) {
        printf("%x ", buffer2[i]);
    }
    printf("\n");

}