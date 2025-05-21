#ifndef _VFS_H
#define _VFS_H

#include <omen/libraries/std/stdint.h>
#include <omen/managers/dev/devices.h>
#include "generic/vfs_compat.h"

/// MBR and boot sector
#define MBR_BOOTSTRAP		0
#define MBR_BOOTSTRAP_SIZE	446
#define MBR_PARTITION		446
#define MBR_PARTITION_SIZE	16
#define MBR_BOOT_SIG		510
#define MBR_BOOT_SIG_VALUE	0xAA55

#define VFS_MAX_DEVICES 512

#define VFS_ERROR (-1)

#define PAR_STATUS			0
#define PAR_TYPE			4
#define PAR_LBA				8
#define PAR_SIZE			12

struct vfs_file_system_type {
	char name[32];
    char majors[256];
    int major_no;
    int (*register_partition)(const char*, uint32_t, const char*);
    int8_t (*unregister_partition)(int);
    uint8_t (*detect)(const char *, uint32_t);
    int (*flush)(int);

    int (*file_flush)(int, int);
    int (*file_open)(int, const char*, int, int);
    int (*file_close)(int, int);
    int (*file_creat)(int, const char*, int);
    int (*file_dup)(int, int, int);
    int64_t (*file_read)(int, int, void*, uint64_t);
    int64_t (*file_write)(int, int, void*, uint64_t);
    int64_t (*file_seek)(int, int, uint64_t, int);
    int64_t (*file_ioctl)(int, int, int, void*);
    int64_t (*file_tell)(int, int);
    int (*file_stat)(int, int, stat_t*);

    int (*dir_read)(int, int, char*, uint32_t *, uint32_t *);
    int (*dir_open)(int, const char*);
    int (*dir_close)(int, int);
    int (*dir_load)(int, int);
    int (*dir_creat)(int, const char*, int);
    int (*prepare_remove)(int partno, const char* path);


    int (*rename)(int, const char*, const char*);
    int (*remove)(int, const char*);
    int (*chmod)(int, const char*, int);
    
    void (*debug)(void);
    //TODO: Ampliar hasta cubrir generic/compat_vfs
    struct vfs_file_system_type * next;
};

struct vfs_struct {
    struct path root;
    struct path pwd;
};

struct vfs_partition {
    char name[48];
	uint32_t lba;
	uint32_t size;
	uint8_t status;
	uint8_t type;
    struct vfs_partition* next;
};

struct vfs_devmap {
    struct device * dev;
    struct vfs_partition * partitions;
    uint32_t partition_no;
};

struct vfs_mount {
    struct device* device;
    struct vfs_partition* partition;
    struct vfs_file_system_type* fst;
    int internal_index;
    struct vfs_mount * next;
};

struct vfs_dentry {
    uint32_t inode;                 /* Inode number */
    uint16_t rec_len;               /* Directory entry length */
    uint8_t  name_len;              /* Name length */
    uint8_t  file_type;             /* File type */
    char     name[255];   /* File name */ //TODO: CONVERT THIS TO A POINTER
} __attribute__((packed));

void dump_mounts();
uint8_t iterate_mounts(uint8_t (*callback)(struct vfs_mount*, void * data, void* passthrough), void * data, void* passthrough);
struct vfs_mount* get_mount_from_path(const char* path, char* native_path);
struct vfs_dentry* get_dentry_from_path(const char* path, char* native_path);
char * get_path_from_mount_and_dentry(struct vfs_mount * mount, struct vfs_dentry * dentry);
char* get_full_path_from_fd(int fd);
char* get_full_path_from_dir(int fd);
char* get_full_path_from_dev(int fd);
int is_safe_for_removing(const char* path, uint8_t force);
void register_filesystem(struct vfs_compatible *);
void set_main_mount(const char * mountname);
uint8_t is_absolute_path(const char* path);
uint8_t goes_behind_root(const char* path);
char * apply_cwd(struct vfs_struct * cwd, const char* path, uint64_t * size);
void probe_fs();
#endif