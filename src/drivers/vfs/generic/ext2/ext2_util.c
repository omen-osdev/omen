#pragma GCC diagnostic ignored "-Wvariadic-macros"

#include <time.h>

#include "ext2_util.h"

#include <omen/libraries/std/string.h>
#include <omen/libraries/allocators/heap_allocator.h>

#define EXT2_UNIQUE_START       0x00CAFE00

uint32_t ext2_unique_id = EXT2_UNIQUE_START;

uint8_t * ext2_buffer_for_size(uint32_t block_size, uint64_t size) {
    uint32_t blocks_for_size = DIVIDE_ROUNDED_UP(size, block_size);
    uint32_t buffer_size = blocks_for_size * block_size;

    return kmalloc(buffer_size);
}

void epoch_to_date(char* date, uint32_t epoch) {
    time_t t = epoch;
    struct tm *tm = localtime(&t);
    strftime(date, 32, "%Y-%m-%d %H:%M:%S", tm);
}

uint32_t ext2_get_current_epoch() {
    time_t t = time(0);
    return (uint32_t)t;
}

uint32_t ext2_get_uid() {
    return (uint32_t)0;//getuid();
}

uint32_t ext2_get_gid() {
    return (uint32_t)0;//getgid();
}

uint32_t ext2_get_unique_id() {
    //Check for overflow
    if (ext2_unique_id == 0xFFFFFFFF) {
        ext2_unique_id = EXT2_UNIQUE_START;
    } else {
        ext2_unique_id++;
    }

    return ext2_unique_id;
}

uint8_t ext2_path_to_parent_and_name(const char* source, char** path, char** name) {
    uint64_t path_length = strlen(source);
    if (path_length == 0) return 0;

    uint64_t last_slash = 0;
    for (uint64_t i = 0; i < path_length; i++) {
        if (source[i] == '/') last_slash = i;
    }

    if (last_slash == 0) {
        *path = (char*)kmalloc(2);
        (*path)[0] = '/';
        (*path)[1] = 0;
        *name = (char*)kmalloc(path_length + 1);
        memcpy(*name, source+1, path_length + 1);
        return 1;
    }

    *path = (char*)kmalloc(last_slash + 1);
    memcpy(*path, source, last_slash);
    (*path)[last_slash] = 0;

    *name = (char*)kmalloc(path_length - last_slash);
    memcpy(*name, source + last_slash + 1, path_length - last_slash);
    return 1;
}