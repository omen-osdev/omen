#ifndef _VFS_COMPAT_H
#define _VFS_COMPAT_H
#include <omen/libraries/std/stdint.h>
#define VFS_COMPAT_FS_NAME_MAX_LEN 32
#define VFS_COMPAT_MAX_OPEN_FILES 65536
#define VFS_COMPAT_MAX_OPEN_DIRECTORIES 4096
#define VFS_FDE_NAME_MAX_LEN 256

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002

#define O_APPEND 0x0008
#define O_CREAT 0x0200
#define O_TRUNC 0x0400
#define O_EXCL 0x0800
#define O_NONBLOCK 0x4000

#define O_ISAPPEND(flags) ((flags) & O_APPEND)
#define O_ISCREAT(flags) ((flags) & O_CREAT)
#define O_ISTRUNC(flags) ((flags) & O_TRUNC)
#define O_ISEXCL(flags) ((flags) & O_EXCL)
#define O_ISNONBLOCK(flags) ((flags) & O_NONBLOCK)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define S_IFMT 0170000
#define S_IFSOCK 0140000
#define S_IFLNK 0120000
#define S_IFREG 0100000
#define S_IFBLK 0060000
#define S_IFDIR 0040000
#define S_IFCHR 0020000
#define S_IFIFO 0010000
#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000
#define S_IRWXU 0000700
#define S_IRUSR 0000400
#define S_IWUSR 0000200
#define S_IXUSR 0000100
#define S_IRWXG 0000070
#define S_IRGRP 0000040
#define S_IWGRP 0000020
#define S_IXGRP 0000010
#define S_IRWXO 0000007
#define S_IROTH 0000004
#define S_IWOTH 0000002
#define S_IXOTH 0000001

#define S_ISLNK(m) (((m)&S_IFMT) == S_IFLNK)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#define S_ISCHR(m) (((m)&S_IFMT) == S_IFCHR)
#define S_ISBLK(m) (((m)&S_IFMT) == S_IFBLK)
#define S_ISFIFO(m) (((m)&S_IFMT) == S_IFIFO)
#define S_ISSOCK(m) (((m)&S_IFMT) == S_IFSOCK)

struct stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_mode;
    uint64_t st_nlink;
    uint64_t st_uid;
    uint64_t st_gid;
    uint64_t st_rdev;
    uint64_t st_size;
    uint64_t st_blksize;
    uint64_t st_blocks;
    uint64_t st_atime;
    uint64_t st_mtime;
    uint64_t st_ctime;
};
typedef struct stat stat_t;

struct file_descriptor_entry {
    char name[VFS_FDE_NAME_MAX_LEN];
    char mount[VFS_FDE_NAME_MAX_LEN];
    uint32_t flags;
    uint32_t mode;
    uint64_t offset;
    uint8_t loaded;
};

struct dentry {
    char name[VFS_FDE_NAME_MAX_LEN];
    uint32_t name_len;
    uint32_t inode;
    uint32_t type;
    struct dentry *next;
};

struct dir {
    struct file_descriptor_entry fd;
    struct dentry *dentries;
    uint32_t number;
    uint32_t index;
};

typedef struct dir dir_t;

struct vfs_compatible {
    char name[VFS_COMPAT_FS_NAME_MAX_LEN];
    char majors[256];
    int major_no;
    int (*register_partition)(const char*, uint32_t, const char*);
    int8_t (*unregister_partition)(int);
    uint8_t (*detect)(const char*, uint32_t);
    int (*flush)(int);

    int (*file_flush)(int, int);
    int (*file_open)(int, const char*, int, int);
    int (*file_close)(int, int);
    int (*file_creat)(int, const char*, int);
    int (*file_link)(int, const char*, const char*, int);
    int (*file_dup)(int, int, int);
    int64_t (*file_read)(int, int, void*, uint64_t);
    int64_t (*file_write)(int, int, void*, uint64_t);
    int64_t (*file_seek)(int, int, uint64_t, int);
    int64_t (*file_tell)(int, int);
    int64_t (*file_ioctl)(int, int, int, void*);

    int (*file_stat)(int, int, stat_t*);

    int (*dir_open)(int, const char*);
    int (*dir_close)(int, int);
    int (*dir_load)(int, int);
    int (*dir_read)(int, int, char*, uint32_t *, uint32_t *);
    int (*dir_creat)(int, const char*, int);
    
    int (*prepare_remove)(int partno, const char* path);
    
    char* (*file_readlink)(int, const char*);

    int (*rename)(int, const char*, const char*);
    int (*remove)(int, const char*);
    int (*chmod)(int, const char*, int);
    
    void (*debug)(void);

    //TODO: Link operations!!!

};

dir_t * vfs_compat_get_dir(int fd);
struct file_descriptor_entry * vfs_compat_get_file_descriptor(int fd);

int get_fd(const char* path, const char* mount, int flags, int mode);
int get_dirfd(const char* path, const char* mount, int flags, int mode);
int dup_fd(int oldfd, int newfd); //If newfd == VFS_ERROR use dup, else use dup2

uint8_t add_file_to_dirfd(int fd, const char* name, uint32_t inode, uint32_t type, uint32_t name_len);
int release_dirfd(int fd);
int release_fd(int fd);
int force_release(const char * path);

int is_open(const char* path);
int is_empty(const char* path);
int read_dirfd(int fd, char * name, uint32_t * name_len, uint32_t * type); 
#endif