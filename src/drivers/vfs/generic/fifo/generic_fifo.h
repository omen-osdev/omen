#ifndef _GENERIC_VFS_FIFO_H
#define _GENERIC_VFS_FIFO_H

#include "fifo.h"

#include "../vfs_compat.h"

#include <omen/libraries/std/string.h>
#include <omen/apps/debug/debug.h>

#define MAX_FIFO_DEVICES 32

struct vfs_fifo* fifo_devices[MAX_FIFO_DEVICES] = {0};
//-1 cannot return the object
// 0 all okey
// 1 error

int fifo_compat_register_device(const char* device, uint32_t mode, const char* mountpoint) {
    for (int i = 0; i < MAX_FIFO_DEVICES; i++) {
        if (fifo_devices[i] == 0) {
            fifo_devices[i] = fifo_register_device(device, mode, mountpoint);
            if (fifo_devices[i] == 0) {
                return -1;
            }
            
            return i;
        }
    }
    return -1;
}

uint8_t fifo_compat_unregister_device(int index) {
    if (fifo_devices[index] != 0) {
        fifo_unregister_device(fifo_devices[index]);
        fifo_devices[index] = 0;
        return 0;
    }
    return 1;
}

uint8_t fifo_compat_detect(const char* name , uint32_t port) {
    (void)port;
    kprintf("fifo_compat_detect: %s\n", name);
    return (fifo_search(name) == SUCCESS);
}

void fifo_compat_debug() {
    for (int i = 0; i < MAX_FIFO_DEVICES; i++) {
        if (fifo_devices[i] != 0) {
            kprintf("FIFO %d\n", i);
            fifo_dump_device(fifo_devices[i]);
        }
    }
}

uint64_t fifo_compat_file_read(int devno, int fd, void* buffer, uint64_t size) {
    if (devno < 0 || devno >= MAX_FIFO_DEVICES) 
        return -1;

    struct vfs_fifo * device = fifo_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;
    return vfs_fifo_read(device, buffer, size, entry->offset);
}

uint64_t fifo_compat_file_write(int devno, int fd, void* buffer, uint64_t size) {
    if (devno < 0 || devno >= MAX_FIFO_DEVICES) 
        return -1;

    struct vfs_fifo * device = fifo_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;
    return vfs_fifo_write(device, buffer, size, entry->offset);
}

uint64_t fifo_compat_file_seek(int devno, int fd, uint64_t offset, int whence) {
    if (devno < 0 || devno >= MAX_FIFO_DEVICES) 
        return -1;

    struct vfs_fifo * device = fifo_devices[devno];
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
            size = vfs_fifo_get_size(device);
            entry->offset = size + offset;
            break;
        }
        default:
            return 0;
    }

    return entry->offset;
}

uint64_t fifo_compat_file_tell(int devno, int fd) {
    if (devno < 0 || devno >= MAX_FIFO_DEVICES) 
        return -1;

    struct vfs_fifo * device = fifo_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;
    return entry->offset;
}

int fifo_compat_file_open(int devno, const char* path, int flags, int mode) {
    if (devno < 0 || devno >= MAX_FIFO_DEVICES) 
        return -1;

    struct vfs_fifo * device = fifo_devices[devno];
    if (device == 0)
        return -1;

    if (strlen(path) != 0) return -1;

    return get_fd(path, device->name, flags, mode);
}

uint64_t fifo_compat_file_ioctl(int devno, int fd, int request, void* arg) {
    if (devno < 0 || devno >= MAX_FIFO_DEVICES) 
        return -1;

    struct vfs_fifo * device = fifo_devices[devno];
    if (device == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;

    return vfs_fifo_ioctl(device, request, arg);
}

int fifo_compat_file_close(int devno, int fd) {
    if (devno < 0 || devno >= MAX_FIFO_DEVICES) 
        return -1;

    struct vfs_fifo * device = fifo_devices[devno];
    if (device == 0)
        return -1;

    return release_fd(fd);
}

int fifo_compat_file_flush(int devno, int fd) {
    if (devno < 0 || devno >= MAX_FIFO_DEVICES) 
        return -1;

    struct vfs_fifo * device = fifo_devices[devno];
    return (int)fifo_sync(device);
}

int fifo_compat_flush(int partno) {(void)partno; return -1;}
int fifo_compat_dir_open(int partno, const char* path) {(void)partno; (void)path; return -1;}
int fifo_compat_dir_close(int partno, int fd) {(void)partno; (void)fd; return -1;}
int fifo_compat_file_creat(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}
int fifo_compat_dir_creat(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}
int fifo_compat_dir_read(int partno, int fd, char* name, uint32_t * name_len, uint32_t * type) {(void)partno; (void)fd; (void)name_len; (void)type; return -1;}
int fifo_compat_dir_load(int partno, int fd) {(void)partno; (void)fd; return -1;}
int fifo_compat_stat(int partno, int fd, stat_t* st) {(void)partno; (void)fd; (void)st; return -1;}
int fifo_compat_rename(int partno, const char* path, const char* newpath) {(void)partno; (void)path; (void)newpath; return -1;}
int fifo_compat_prepare_remove(int partno, const char* path) {(void)partno; (void)path; return -1;}
int fifo_compat_remove(int partno, const char* path) {(void)partno; (void)path; return -1;}
int fifo_compat_chmod(int partno, const char* path, int mode) {(void)partno; (void)path; (void)mode; return -1;}

struct vfs_compatible fifo_register = {
    .name = "FIFO",
    .majors = {0x8e},
    .major_no = 1,
    .register_partition = fifo_compat_register_device,
    .unregister_partition = fifo_compat_unregister_device,
    .detect = fifo_compat_detect,
    .flush = fifo_compat_flush,
    .file_flush = fifo_compat_file_flush,
    .debug = fifo_compat_debug,
    .file_open = fifo_compat_file_open,
    .file_close = fifo_compat_file_close,
    .file_creat = fifo_compat_file_creat,
    .file_read = fifo_compat_file_read,
    .file_write = fifo_compat_file_write,
    .file_seek = fifo_compat_file_seek,
    .file_tell = fifo_compat_file_tell,
    .file_stat = fifo_compat_stat,
    .file_ioctl = fifo_compat_file_ioctl,
    .rename = fifo_compat_rename,
    .remove = fifo_compat_remove,
    .chmod = fifo_compat_chmod,
    .dir_open = fifo_compat_dir_open,
    .dir_close = fifo_compat_dir_close,
    .dir_creat = fifo_compat_dir_creat,
    .dir_read = fifo_compat_dir_read,
    .dir_load = fifo_compat_dir_load,
    .prepare_remove = fifo_compat_prepare_remove
};

struct vfs_compatible * fifo_registrar = &fifo_register;

#endif