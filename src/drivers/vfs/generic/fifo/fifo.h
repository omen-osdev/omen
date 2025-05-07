#ifndef _VFS_FIFO_H
#define _VFS_FIFO_H

#include <omen/libraries/std/stdint.h>
#include <serial/serial.h>

struct vfs_fifo {
    char name[32];
    char device[32];
};

struct vfs_fifo* fifo_register_device(const char * device, uint32_t mode, const char * mountpoint);
void fifo_unregister_device(struct vfs_fifo* fifo);
uint8_t fifo_search(const char* name);
int fifo_sync(struct vfs_fifo* fifo);
void fifo_dump_device(struct vfs_fifo* fifo);
int64_t vfs_fifo_get_size(struct vfs_fifo* fifo);
int64_t vfs_fifo_read(struct vfs_fifo * fifo, uint8_t * destination_buffer, uint64_t size, uint64_t skip);
int64_t vfs_fifo_write(struct vfs_fifo * fifo, uint8_t * source_buffer, uint64_t size, uint64_t skip);
int64_t vfs_fifo_ioctl(struct vfs_fifo * fifo, uint32_t request, uint32_t arg);
#endif