#ifndef _VFS_TTY_H
#define _VFS_TTY_H

#include <omen/libraries/std/stdint.h>
#include <serial/serial.h>

struct vfs_tty {
    char name[32];
    char device[32];
};

struct vfs_tty* tty_register_device(const char * device, uint32_t mode, const char * mountpoint);
void tty_unregister_device(struct vfs_tty* tty);
uint8_t tty_search(const char* name);
int tty_sync(struct vfs_tty* tty);
void tty_dump_device(struct vfs_tty* tty);
uint64_t vfs_tty_has_input(struct vfs_tty* tty);
uint64_t vfs_tty_get_size(struct vfs_tty* tty);
uint8_t vfs_tty_read(struct vfs_tty * tty, uint8_t * destination_buffer, uint64_t size, uint64_t skip);
uint8_t vfs_tty_write(struct vfs_tty * tty, uint8_t * source_buffer, uint64_t size, uint64_t skip);
uint8_t vfs_tty_ioctl(struct vfs_tty * tty, int request, void* arg);
#endif