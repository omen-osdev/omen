#ifndef _EXT2_UTIL_H
#define _EXT2_UTIL_H

#include <omen/libraries/std/stdint.h>

//Fix by FunkyWaddle: Inverted values
#define DIVIDE_ROUNDED_UP(x, y) ((x % y) ? ((x) + (y) - 1) / (y) : (x) / (y))
#define SWAP_ENDIAN_16(x) (((x) >> 8) | ((x) << 8))

uint8_t * ext2_buffer_for_size(uint32_t block_size, uint64_t size);
void epoch_to_date(char* date, uint32_t epoch);
uint32_t ext2_get_unique_id();
uint32_t ext2_get_current_epoch();
uint8_t ext2_path_to_parent_and_name(const char* source, char** path, char** name);
uint32_t ext2_get_uid();
uint32_t ext2_get_gid();
#endif /* _EXT2_UTIL_H */