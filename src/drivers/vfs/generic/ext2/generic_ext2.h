#ifndef _GENERIC_EXT2_H
#define _GENERIC_EXT2_H

#include "ext2.h"

#include "ext2_structs.h"

#include <vfs/generic/vfs_compat.h>
#include <omen/apps/debug/debug.h>

#define MAX_EXT2_PARTITIONS 32
#define MAX_EXT2_OPEN_FILES 65536

struct ext2_partition* ext2_partitions[MAX_EXT2_PARTITIONS] = {0};
//-1 cannot return the object
// 0 all okey
// 1 error

int ext2_compat_register_partition(const char* drive, uint32_t lba, const char* mountpoint) {
    for (int i = 0; i < MAX_EXT2_PARTITIONS; i++) {
        if (ext2_partitions[i] == 0) {
            ext2_partitions[i] = ext2_register_partition(drive, lba, mountpoint);
            if (ext2_partitions[i] == 0) {
                return -1;
            }
            return i;
        }
    }
    return -1;
}

uint8_t ext2_compat_unregister_partition(int index) {
    if (ext2_partitions[index] != 0) {
        ext2_unregister_partition(ext2_partitions[index]);
        ext2_partitions[index] = 0;
        return 0;
    }
    return 1;
}

uint8_t ext2_compat_detect(const char* disk , uint32_t lba) {

    return ext2_search(disk, lba);
}

int ext2_compat_flush(int index) {
    if (index < 0 || index >= MAX_EXT2_PARTITIONS)
        return -1;

    struct ext2_partition* partition = ext2_partitions[index];
    return (int)ext2_sync(partition);
}

void ext2_compat_debug() {
    for (int i = 0; i < MAX_EXT2_PARTITIONS; i++) {
        if (ext2_partitions[i] != 0) {
            kprintf("Partition %d\n", i);
            ext2_dump_partition(ext2_partitions[i]);
        }
    }
}

int ext2_compat_file_open(int partno, const char* path, int flags, int mode) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    kprintf("opening file: partno: %d, path: %s\n", partno, path);
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;
    
    return get_fd(path, partition->name, flags, mode);
}


int ext2_compat_dir_open(int partno, const char* path) {
    
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    kprintf("opening directory: partno: %d, path: %s\n", partno, path);
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;

    return get_dirfd(path, partition->name, 0, 0);
}

int ext2_compat_file_close(int partno, int fd) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;

    return release_fd(fd);
}

int ext2_compat_dir_close(int partno, int fd) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;

    return release_dirfd(fd);
}


int ext2_compat_file_creat(int partno, const char* path, int mode) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    kprintf("creating file: partno: %d, path: %s in mode: %d\n", partno, path, mode);
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;
    
    //uint8_t ext2_create_file(struct ext2_partition * partition, const char* path, uint32_t type, uint32_t permissions);
    uint8_t res = ext2_create_file(partition, path, EXT2_FILE_TYPE_REGULAR, mode);
    if (res != EXT2_RESULT_OK) {
        return -1;
    }

    return get_fd(path, partition->name, 0, mode);
}

int ext2_compat_file_dup(int partno, int oldfd, int newfd) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(oldfd);
    if (entry == 0 || entry->loaded == 0) return -1;
    return dup_fd(oldfd, newfd);
}


int ext2_compat_dir_creat(int partno, const char* path, int mode) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    kprintf("creating directory: partno: %d, path: %s in mode: %d\n", partno, path, mode);
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;
    
    uint8_t res = ext2_create_file(partition, path, EXT2_FILE_TYPE_DIRECTORY, mode);
    if (res != EXT2_RESULT_OK) {
        return -1;
    }

    return get_dirfd(path, partition->name, 0, mode);
}

uint64_t ext2_compat_file_read(int partno, int fd, void* buffer, uint64_t size) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return 0;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return 0;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return 1;
    return ext2_read_file(partition, entry->name, buffer, size, entry->offset);
}

int ext2_compat_dir_read(int partno, int fd, char* name, uint32_t * name_len, uint32_t * type) {
    (void)partno;
    return read_dirfd(fd, name, name_len, type);
}

int ext2_compat_dir_load(int partno, int fd) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return 0;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return 0;

    dir_t * dir_entry = vfs_compat_get_dir(fd);
    if (dir_entry == 0) return 0;
    struct file_descriptor_entry * entry = &(dir_entry->fd);
    if (entry == 0 || entry->loaded == 0) return 0;

    struct ext2_directory_entry * entries;
    uint32_t count = 0;
    uint8_t result = ext2_read_directory(partition, entry->name, &count, &entries);
    if (result != EXT2_RESULT_OK) {
        //kprintf("(ext2_compat_dir_load) Error reading directory\n");
        return 0;
    }

    //kprintf("(ext2_compat_dir_load) count: %d Loading directory: %s\n", count, entry->name);

    for (uint32_t i = 0; i < count; i++) {
        //kprintf("Adding file to dirfd: %s\n", entries[i].name);
        struct ext2_directory_entry * entry = &entries[i];
        add_file_to_dirfd(fd, entry->name, entry->inode, entry->file_type, entry->name_len);
    }

    return 1;
}

uint64_t ext2_compat_file_seek(int partno, int fd, uint64_t offset, int whence) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return 0;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return 0;
    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
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
            size = ext2_get_file_size(partition, entry->name);
            entry->offset = size + offset;
            break;
        }
        default:
            return 0;
    }

    return entry->offset;
}

uint64_t ext2_compat_file_write(int partno, int fd, void* buffer, uint64_t size) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    uint64_t written = ext2_write_file(partition, entry->name, buffer, size, entry->offset);
    ext2_compat_file_seek(partno, fd, size, SEEK_CUR);
    return written;
}

uint64_t ext2_compat_file_tell(int partno, int fd) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return 0;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return 0;
    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    return entry->offset;
}

int ext2_compat_stat(int partno, int fd, stat_t* st) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return -1;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return -1;

    struct file_descriptor_entry * entry = vfs_compat_get_file_descriptor(fd);
    if (entry == 0 || entry->loaded == 0) return -1;

    uint8_t result = ext2_get_stat(partition, entry->name, st);
    if (result != EXT2_RESULT_OK) {
        return -1;
    }

    return 0;
}

int ext2_compat_rename(int partno, const char* path, const char* newpath) {(void)partno;(void)path;(void)newpath; return -1;}

int ext2_compat_prepare_remove(int partno, const char* path) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return 0;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return 0;
   

    struct ext2_directory_entry entry;
    uint8_t result = ext2_get_dentry(partition, path, &entry);
    if (result != EXT2_RESULT_OK) {
        return 0;
    }

    if (is_directory(&entry)) {
        uint32_t count = 0;
        struct ext2_directory_entry * entries;
        result = ext2_read_directory(partition, path, &count, &entries);
        if (result != EXT2_RESULT_OK) {
            return 0;
        }

        return (count == 0);
    }

    return 1;
}

int ext2_compat_remove(int partno, const char* path) {
    if (partno < 0 || partno >= MAX_EXT2_PARTITIONS) 
        return 0;
    struct ext2_partition * partition = ext2_partitions[partno];
    if (partition == 0)
        return 0;

    return ext2_delete_file(partition, path);
}
int ext2_compat_chmod(int partno, const char* path, int mode) {(void)partno;(void)path;(void)mode;return -1;}
uint64_t ext2_compat_ioctl(int partno, int fd, int request, void * args) {(void)partno;(void)fd;(void)request;(void)args;return -1;}

struct vfs_compatible ext2_register = {
    .name = "EXT2",
    .majors = {0x8, 0x9, 0xa, 0xb, 0xc},
    .major_no = 5,
    .register_partition = ext2_compat_register_partition,
    .unregister_partition = ext2_compat_unregister_partition,
    .detect = ext2_compat_detect,
    .flush = ext2_compat_flush,
    .debug = ext2_compat_debug,
    .file_open = ext2_compat_file_open,
    .file_close = ext2_compat_file_close,
    .file_creat = ext2_compat_file_creat,
    .file_dup = ext2_compat_file_dup,
    .file_read = ext2_compat_file_read,
    .file_write = ext2_compat_file_write,
    .file_seek = ext2_compat_file_seek,
    .file_tell = ext2_compat_file_tell,
    .file_stat = ext2_compat_stat,
    .file_ioctl = ext2_compat_ioctl,
    .rename = ext2_compat_rename,
    .remove = ext2_compat_remove,
    .chmod = ext2_compat_chmod,
    .dir_open = ext2_compat_dir_open,
    .dir_close = ext2_compat_dir_close,
    .dir_creat = ext2_compat_dir_creat,
    .dir_read = ext2_compat_dir_read,
    .dir_load = ext2_compat_dir_load,
    .prepare_remove = ext2_compat_prepare_remove
};

struct vfs_compatible * ext2_registrar = &ext2_register;

#endif