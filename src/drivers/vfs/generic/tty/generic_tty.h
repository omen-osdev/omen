#ifndef _GENERIC_TTY_H
#define _GENERIC_TTY_H

#include "tty.h"

#include "../vfs_compat.h"

#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/std/stdint.h>

#define MAX_TTY_DEVICES 32

struct vfs_tty* tty_devices[MAX_TTY_DEVICES] = {0};
//-1 cannot return the object
// 0 all okey
// 1 error

int tty_compat_register_device(const char* device, uint32_t mode, const char* mountpoint) {
    for (int i = 0; i < MAX_TTY_DEVICES; i++) {
        if (tty_devices[i] == 0) {
            tty_devices[i] = tty_register_device(device, mode, mountpoint);
            if (tty_devices[i] == 0) {
                return -1;
            }
            
            return i;
        }
    }
    return -1;
}

uint8_t tty_compat_unregister_device(int index) {
    if (tty_devices[index] != 0) {
        tty_unregister_device(tty_devices[index]);
        tty_devices[index] = 0;
        return 0;
    }
    return 1;
}

uint8_t tty_compat_detect(const char* name , uint32_t port) {
    (void)port;
    kprintf("tty_compat_detect: %s\n", name);
    return (tty_search(name) == SUCCESS);
}

void tty_compat_debug() {
    for (int i = 0; i < MAX_TTY_DEVICES; i++) {
        if (tty_devices[i] != 0) {
            kprintf("TTY %d\n", i);
            tty_dump_device(tty_devices[i]);
        }
    }
}

uint64_t tty_compat_file_read(int devno, int fd, void* buffer, uint64_t size) {
    if (devno < 0 || devno >= MAX_TTY_DEVICES) 
        return -1;

    struct vfs_tty * device = tty_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;
    if (O_ISNONBLOCK(entry->flags)) {
        return vfs_tty_read(device, buffer, size, entry->offset);
    } else {
        uint64_t read = 0;
        while (read < size) {
            uint64_t res = vfs_tty_read(device, buffer + read, size - read, entry->offset);
            if (res != 0) kprintf("tty read %d bytes\n", res);
            read += res;
        }
        return read;
    }
}

uint64_t tty_compat_file_write(int devno, int fd, void* buffer, uint64_t size) {
    if (devno < 0 || devno >= MAX_TTY_DEVICES) 
        return -1;

    struct vfs_tty * device = tty_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;
    return vfs_tty_write(device, buffer, size, entry->offset);
}

uint64_t tty_compat_file_seek(int devno, int fd, uint64_t offset, int whence) {
    if (devno < 0 || devno >= MAX_TTY_DEVICES) 
        return -1;

    struct vfs_tty * device = tty_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;

    uint64_t size = 0;
    switch (whence) {
        case SEEK_SET: {
            entry->offset = offset;
            break;
        }
        case SEEK_CUR: {
            entry->offset += offset;
            break;
        }
        case SEEK_END: {
            size = vfs_tty_get_size(device);
            entry->offset = size + offset;
            break;
        }
        default:
            return 0;
    }

    return entry->offset;
}

uint64_t tty_compat_file_tell(int devno, int fd) {
    if (devno < 0 || devno >= MAX_TTY_DEVICES) 
        return -1;

    struct vfs_tty * device = tty_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;
    return entry->offset;
}

uint64_t tty_compat_file_ioctl(int devno, int fd, int request, void* arg) {
    if (devno < 0 || devno >= MAX_TTY_DEVICES) 
        return -1;

    struct vfs_tty * device = tty_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;
    return vfs_tty_ioctl(device, request, arg);
}

int tty_compat_file_open(int devno, const char* path, int flags, int mode) {
    if (devno < 0 || devno >= MAX_TTY_DEVICES) 
        return -1;

    struct vfs_tty * device = tty_devices[devno];
    if (device == 0)
        return -1;

    if (strlen(path) != 0) return -1;

    return get_fd(path, device->name, flags, mode);
}

int tty_compat_file_close(int devno, int fd) {
    if (devno < 0 || devno >= MAX_TTY_DEVICES) 
        return -1;

    struct vfs_tty * device = tty_devices[devno];
    if (device == 0)
        return -1;

    return release_fd(fd);
}

int tty_compat_file_flush(int devno, int fd) {
    if (devno < 0 || devno >= MAX_TTY_DEVICES) 
        return -1;

    struct vfs_tty * device = tty_devices[devno];
    return (int)tty_sync(device);
}

int tty_compat_flush(int partno) {(void)partno; return -1;}
int tty_compat_dir_open(int partno, const char* path) {(void)partno; (void)path; return -1;}
int tty_compat_dir_close(int partno, int fd) {(void)partno; (void)fd; return -1;}
int tty_compat_file_creat(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}
int tty_compat_dir_creat(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}
int tty_compat_dir_read(int partno, int fd, char* name, uint32_t * name_len, uint32_t * type) {(void)partno; (void)fd; (void)name_len; (void)type; return -1;}
int tty_compat_dir_load(int partno, int fd) {(void)partno; (void)fd; return -1;}
int tty_compat_stat(int partno, int fd, stat_t* st) {(void)partno; (void)fd; (void)st; return -1;}
int tty_compat_rename(int partno, const char* path, const char* newpath) {(void)partno; (void)path; (void)newpath; return -1;}
int tty_compat_prepare_remove(int partno, const char* path) {(void)partno; (void)path; return -1;}
int tty_compat_remove(int partno, const char* path) {(void)partno; (void)path; return -1;}
int tty_compat_chmod(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}

struct vfs_compatible tty_register = {
    .name = "TTY",
    .majors = {0xE},
    .major_no = 1,
    .register_partition = tty_compat_register_device,
    .unregister_partition = tty_compat_unregister_device,
    .detect = tty_compat_detect,
    .flush = tty_compat_flush,
    .file_flush = tty_compat_file_flush,
    .debug = tty_compat_debug,
    .file_open = tty_compat_file_open,
    .file_close = tty_compat_file_close,
    .file_creat = tty_compat_file_creat,
    .file_read = tty_compat_file_read,
    .file_write = tty_compat_file_write,
    .file_seek = tty_compat_file_seek,
    .file_tell = tty_compat_file_tell,
    .file_stat = tty_compat_stat,
    .file_ioctl = tty_compat_file_ioctl,
    .rename = tty_compat_rename,
    .remove = tty_compat_remove,
    .chmod = tty_compat_chmod,
    .dir_open = tty_compat_dir_open,
    .dir_close = tty_compat_dir_close,
    .dir_creat = tty_compat_dir_creat,
    .dir_read = tty_compat_dir_read,
    .dir_load = tty_compat_dir_load,
    .prepare_remove = tty_compat_prepare_remove
};

struct vfs_compatible * tty_registrar = &tty_register;

#endif