#ifndef _VFS_ADAPTERS_H
#define _VFS_ADAPTERS_H
#include <omen/libraries/std/stdint.h>
#include <vfs/generic/vfs_compat.h>
#include "vfs.h"

void vfs_lsdisk();

int vfs_socket_open(int, int, int);

int vfs_file_open(struct vfs_struct * cwd, struct char* path, int, int);
int vfs_file_dup(int oldfd, int newfd);
int vfs_file_search(struct vfs_struct * cwd, const char * name, char * path);
int vfs_file_close(int);
int64_t vfs_file_read(int, void*, uint64_t);
int64_t vfs_file_write(int, void*, uint64_t);
int64_t vfs_file_seek(int, uint64_t, int);
int64_t vfs_file_tell(int);
int64_t vfs_file_ioctl(int, int, void*);
int vfs_file_stat(int, stat_t*);
int vfs_file_creat(struct vfs_struct * cwd, char*, int);
void vfs_file_flush(int);

int vfs_dir_open(struct vfs_struct * cwd, char*);
int vfs_dir_close(int);
int vfs_dir_load(int);
int vfs_mkdir(struct vfs_struct * cwd, char*, int);
int vfs_dir_read(int, char*, uint32_t *, uint32_t *);
void vfs_dir_list(struct vfs_struct * cwd, char*);

int vfs_rename(struct vfs_struct * cwd, char*, const char*);
int vfs_remove(struct vfs_struct * cwd, char*, uint8_t);
int vfs_chmod(struct vfs_struct * cwd, char*, int);
void vfs_debug_by_path(struct vfs_struct * cwd, char*);

int vfs_ioctl(int, int, void*);
#endif