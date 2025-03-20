#ifndef _VFS_ADAPTERS_H
#define _VFS_ADAPTERS_H
#include <omen/libraries/std/stdint.h>

void vfs_lsdisk();

int vfs_socket_open(int, int, int);

int vfs_file_open(char*, int, int);
int vfs_file_search(const char * name, char * path);
int vfs_file_close(int);
uint64_t vfs_file_read(int, void*, uint64_t);
uint64_t vfs_file_write(int, void*, uint64_t);
uint64_t vfs_file_seek(int, uint64_t, int);
uint64_t vfs_file_tell(int);
int vfs_file_creat(char*, int);
void vfs_file_flush(int);

int vfs_dir_open(char*);
int vfs_dir_close(int);
int vfs_dir_load(int);
int vfs_mkdir(char*, int);
int vfs_dir_read(int, char*, uint32_t *, uint32_t *);
void vfs_dir_list(char*);

int vfs_rename(char*, const char*);
int vfs_remove(char*, uint8_t);
int vfs_chmod(char*, int);
void vfs_debug_by_path(char*);

int vfs_ioctl(int, int, void*);
#endif