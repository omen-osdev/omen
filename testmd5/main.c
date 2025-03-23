#include "md5.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    FILE* fd = fopen("../progs/export/init.elf", "rb");
    if (fd < 0) {
        printf("Could not open file %s\n", "../progs/export/init.elf");
        return 1;
    }

    fseek(fd, 0, 2); //SEEK_END
    uint64_t size = ftell(fd);
    fseek(fd, 0, 0); //SEEK_SET

    uint8_t* buf = malloc(size);
    memset(buf, 0, size);

    fread(buf, size, 1, fd);
    fclose(fd);

    unsigned char *md5_buffer = malloc(16);
    memset(md5_buffer, 0, 16);
    MD5_Digest(md5_buffer, buf, size);
    printf("MD5: ");
    for (int i = 0; i < 16; i++) {
        printf("%x", md5_buffer[i]);
    }

    free(buf);
    free(md5_buffer);
    return 0;
}