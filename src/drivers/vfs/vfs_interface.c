#include "vfs_interface.h"
#include "vfs.h"
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>
#include <omen/apps/panic/panic.h>
#include <omen/libraries/std/string.h>
#include <omen/libraries/std/dirent.h>

#define PRINT_ENABLE 0
#define vfs_print(...) if (PRINT_ENABLE) kprintf(__VA_ARGS__)

void vfs_normalize_path(struct vfs_struct * cwd, char * rpath) {
    if (rpath == 0) {
        panic("vfs_normalize_path: Invalid arguments");
    }
    char * path = apply_cwd(cwd, rpath, 0);
    if (path == 0) {
        panic("vfs_normalize_path: Invalid path");
    }
    //kprintf("Normalizing: %s\n", path);
    //Substitute . and .. in the path
    char * path_ptr = path;
    char * path_ptr2 = path;
    while (*path_ptr != 0) {
        if (*path_ptr == '.' && *(path_ptr + 1) == '/') {
            path_ptr += 2;
        } else if (*path_ptr == '.' && *(path_ptr + 1) == '.' && *(path_ptr + 2) == '/') {
            path_ptr += 3;
            while (path_ptr2 != path) {
                path_ptr2--;
                if (*path_ptr2 == '/') {
                    path_ptr2++;
                    break;
                }
            }
        } else {
            *path_ptr2 = *path_ptr;
            path_ptr++;
            path_ptr2++;
        }
    }

    //Remove trailing slashes and dots unless the path is /
    while (path_ptr2 != path) {
        path_ptr2--;
        if (*path_ptr2 != '/' && *path_ptr2 != '.') {
            path_ptr2++;
            break;
        }
    }

    *path_ptr2 = 0;

    //kprintf("Normalized path: %s\n", path);
}

void vfs_lsdisk() {
    dump_mounts();
}

int vfs_socket_open(int family, int type, int protocol) {

}

int vfs_file_dup(int old, int new) {
    vfs_print("vfs_file_dup(%d, %d)\n", old, new);
    char * path = get_full_path_from_fd(old);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_dup(mount->internal_index, old, new);
    }
    kfree(native_path_buffer);
    kfree(path);
    return res;
}

int vfs_file_open(struct vfs_struct * cwd, char* path, int flags, int mode) {
    char * npath = kmalloc(strlen(path) + 1);
    if (flags & O_NOFOLLOW) {
        strcpy(npath, path);
    } else {
        char * symlink_path = vfs_read_symlink(cwd, path);
        if (symlink_path == 0) {
            kfree(npath);
            return VFS_ERROR;
        }
        strcpy(npath, symlink_path);
    }
    vfs_normalize_path(cwd, npath);
    vfs_print("vfs_file_open(%s, %d, %d)\n", npath, flags, mode);
    char * native_path_buffer = kmalloc(strlen(npath) + 1);
    struct vfs_mount* mount = get_mount_from_path(npath, native_path_buffer);
    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_open(mount->internal_index, native_path_buffer, flags, mode);
    }
    kfree(native_path_buffer);
    kfree(npath);
    return res;
}

int vfs_file_close(int fd) {
    vfs_print("vfs_file_close(%d)\n", fd);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return VFS_ERROR;
    }

    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);
    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_close(mount->internal_index, fd);
    }
    kfree(native_path_buffer);
    kfree(path);
    return res;
}

int64_t vfs_file_read(int fd, void* buffer, uint64_t size) {
    vfs_print("vfs_file_read(%d, %p, %ld)\n", fd, buffer, size);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_read(mount->internal_index, fd, (char*)buffer, size);
    }
    kfree(native_path_buffer);
    kfree(path);
    return res;
}

int64_t vfs_file_write(int fd, void* buffer, uint64_t size) {
    vfs_print("vfs_file_write(%d, %p, %ld)\n", fd, buffer, size);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_write(mount->internal_index, fd, (char*)buffer, size);
    }
    kfree(native_path_buffer);
    kfree(path);

    return res;
}

int64_t vfs_file_ioctl(int fd, int request, void* arg) {
    vfs_print("vfs_file_ioctl(%d, %d, %p)\n", fd, request, arg);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    uint64_t res = 0;
    if (mount != 0) {
        res = mount->fst->file_ioctl(mount->internal_index, fd, request, arg);
    }
    kfree(native_path_buffer);
    kfree(path);
    return res;
}

int vfs_file_creat(struct vfs_struct * cwd, char* path, int mode) {
    if (cwd == 0 || path == 0) {
        panic("vfs_file_creat: Invalid arguments");
    }
    char * npath = kmalloc(strlen(path) + 1);
    strcpy(npath, path);
    vfs_normalize_path(cwd, npath);
    vfs_print("vfs_file_creat(%s, %d)\n", npath, mode);
    char * native_path_buffer = kmalloc(strlen(npath) + 1);
    struct vfs_mount* mount = get_mount_from_path(npath, native_path_buffer);
    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_creat(mount->internal_index, native_path_buffer, mode);
    }
    kfree(native_path_buffer);
    kfree(npath);
    return res;
}

int vfs_file_event(int fd, int event_kind, int* event_result) {
    vfs_print("vfs_file_event(%d, %d, %p)\n", fd, event_kind, event_result);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);
    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_event(mount->internal_index, event_kind, event_result);
    }
    kfree(native_path_buffer);
    kfree(path);
    return res;
}

char * vfs_read_symlink(struct vfs_struct * cwd, char * path) {
    vfs_print("vfs_read_symlink(%s)\n", path);
    if (cwd == 0 || path == 0) {
        panic("vfs_read_symlink: Invalid arguments");
    }
    char * npath = kmalloc(strlen(path) + 1);
    strcpy(npath, path);
    vfs_normalize_path(cwd, npath);
    char * native_path_buffer = kmalloc(strlen(npath) + 1);
    struct vfs_mount* mount = get_mount_from_path(npath, native_path_buffer);
    char * res = 0;
    if (mount != 0) {
        res = mount->fst->file_readlink(mount->internal_index, native_path_buffer);
    }
    kfree(native_path_buffer);
    kfree(npath);
    return res;
}

int vfs_link_creat(struct vfs_struct * cwd, char* path, char* target) {
    if (cwd == 0 || path == 0 || target == 0) {
        panic("vfs_link_creat: Invalid arguments");
    }
    char * npath = kmalloc(strlen(path) + 1);
    strcpy(npath, path);
    vfs_normalize_path(cwd, npath);
    vfs_print("vfs_link_creat(%s, %s)\n", npath, target);
    char * native_path_buffer = kmalloc(strlen(npath) + 1);
    struct vfs_mount* mount = get_mount_from_path(npath, native_path_buffer);
    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_link(mount->internal_index, native_path_buffer, target, 0);
    }
    kfree(native_path_buffer);
    kfree(npath);
    return res;
}

int vfs_file_stat(int fd, stat_t* st) {
    vfs_print("vfs_file_stat(%d, %p)\n", fd, st);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->file_stat(mount->internal_index, fd, st);
    }
    kfree(native_path_buffer);
    kfree(path);
    return res;
}

int64_t vfs_file_seek(int fd, uint64_t offset, int whence) {
    vfs_print("vfs_file_seek(%d, %ld, %d)\n", fd, offset, whence);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    uint64_t res = 0;
    if (mount != 0) {
        res = mount->fst->file_seek(mount->internal_index, fd, offset, whence);
    }
    kfree(native_path_buffer);
    kfree(path);
    return res;
}

int64_t vfs_file_tell(int fd) {
    vfs_print("vfs_file_tell(%d)\n", fd);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    uint64_t res = 0;
    if (mount != 0) {
        res = mount->fst->file_tell(mount->internal_index, fd);
    }
    kfree(native_path_buffer);
    kfree(path);
    return res;
}

void* vfs_dir_open(struct vfs_struct * cwd, char* path) {
    if (cwd == 0 || path == 0) {
        panic("vfs_dir_open: Invalid arguments");
    }

    char * npath = kmalloc(strlen(path) + 1);
    strcpy(npath, path);
    vfs_normalize_path(cwd, npath);
    vfs_print("vfs_dir_open(%s)\n", path);
    char * native_path_buffer = kmalloc(strlen(npath) + 1);
    struct vfs_mount* mount = get_mount_from_path(npath, native_path_buffer);
    void * res = 0;
    if (mount != 0) {
        res = mount->fst->dir_open(mount->internal_index, native_path_buffer);
    }
    kfree(native_path_buffer);
    kfree(npath);
    return res;

}

int vfs_dir_close(int fd) {
    vfs_print("vfs_dir_close(%d)\n", fd);

    char * path = get_full_path_from_dir(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->dir_close(mount->internal_index, fd);
    }

    kfree(native_path_buffer);
    kfree(path);
    return res;
}

void vfs_file_flush(int fd) {
    vfs_print("vfs_file_flush(%d)\n", fd);

    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    if (mount != 0) {
        mount->fst->file_flush(mount->internal_index, fd);
    }

    kfree(native_path_buffer);
    kfree(path);
}

int vfs_dir_load(int fd) {
    vfs_print("vfs_dir_load(%d)\n", fd);

    char * path = get_full_path_from_dir(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->dir_load(mount->internal_index, fd);
    }

    kfree(native_path_buffer);
    kfree(path);
    return res;
}

//
//struct vfs_dirent {
//    unsigned long inode;          /* Inode number */
//    unsigned long offset;         /* Offset to the next directory entry */
//    unsigned short reclen;        /* Length of this record */
//    char name[];
//    char pad;
//    char type;
//} __attribute__((packed));

char * get_dirent_name(void * dirent, uint32_t * name_len) {
    if (dirent == 0) {
        return 0;
    }
    //Get the name length
    char * name = (char *) dirent + sizeof(unsigned long) + sizeof(unsigned long) + sizeof(unsigned short);
    *name_len = strlen(name);
    //Return the name
    return name;
}

unsigned long get_dirent_inode(void * dirent) {
    if (dirent == 0) {
        return 0;
    }
    //Get the inode
    unsigned long inode = *(unsigned long *) dirent;
    //Return the inode
    return inode;
}

unsigned long get_dirent_offset(void * dirent) {
    if (dirent == 0) {
        return 0;
    }
    //Get the offset
    unsigned long offset = *(unsigned long *) ((char *) dirent + sizeof(unsigned long));
    //Return the offset
    return offset;
}

unsigned short get_dirent_reclen(void * dirent) {
    if (dirent == 0) {
        return 0;
    }
    //Get the reclen
    unsigned short reclen = *(unsigned short *) ((char *) dirent + sizeof(unsigned long) + sizeof(unsigned long));
    //Return the reclen
    return reclen;
}

unsigned char get_dirent_type(void * dirent) {
    if (dirent == 0) {
        return 0;
    }
    //Get the type
    unsigned char type = *(unsigned char *) ((char *) dirent + sizeof(unsigned long) + sizeof(unsigned long) + sizeof(unsigned short) + strlen((char *) dirent + sizeof(unsigned long) + sizeof(unsigned long) + sizeof(unsigned short)));
    //Return the type
    return type;
}

int vfs_dir_read(int fd, void * dirp, uint32_t count) {
    vfs_print("vfs_dir_read(%d)\n", fd);

    char * path = get_full_path_from_dir(fd);
    if (path == 0) {
        return VFS_ERROR;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    char * name = kmalloc(256);
    uint32_t name_len;
    uint32_t type;
    
    int res = VFS_ERROR;
    uint32_t written = 0;

    if (mount == 0) {
        kfree(native_path_buffer);
        kfree(path);
        return VFS_ERROR;
    }

    do {
        res = mount->fst->dir_read(mount->internal_index, fd, name, &name_len, &type);
        if (res > 0) {
            //kprintf("DIR ENTRY: %s, %d, %d\n", name, type, name_len);

            struct dirent dirent = {0};
            dirent.d_ino = 0;
            dirent.d_off = written;
            dirent.d_reclen = name_len + sizeof(long) + sizeof(long) + sizeof(unsigned short) + sizeof(unsigned char);
            dirent.d_type = type;
            memset(dirent.d_name, 0, 1024);
            strncpy(dirent.d_name, name, name_len);
            //Ensure the name is null-terminated
            dirent.d_name[name_len] = 0;
            memcpy((char *) dirp + written, &dirent, dirent.d_reclen);
            written += dirent.d_reclen;
        }
    } while (res > 0 && written < count);
    //kprintf("DIR READ: %d\n", written);
    kfree(name);
    kfree(native_path_buffer);
    kfree(path);
    return written;
}

int vfs_mkdir(struct vfs_struct * cwd, char* cpath, int mode) {
    if (cwd == 0 || cpath == 0) {
        panic("vfs_mkdir: Invalid arguments");
    }
    char * path = kmalloc(strlen(cpath) + 1);
    strcpy(path, cpath);
    vfs_normalize_path(cwd, path);
    vfs_print("vfs_dir_creat(%s)\n", path);

    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);
    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->dir_creat(mount->internal_index, native_path_buffer, mode);
    }
    kfree(native_path_buffer);
    return res;
}

int vfs_rename(struct vfs_struct * cwd, char* path, const char* name) {return VFS_ERROR;}

int vfs_remove(struct vfs_struct * cwd, char* cpath, uint8_t force) {
    if (cwd == 0 || cpath == 0) {
        panic("vfs_remove: Invalid arguments");
    }
    char * path = kmalloc(strlen(cpath) + 1);
    strcpy(path, cpath);
    vfs_normalize_path(cwd, path);
    vfs_print("vfs_remove(%s)\n", path);
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = VFS_ERROR;
    if (mount != 0) {
        res = mount->fst->prepare_remove(mount->internal_index, native_path_buffer);
        if (res == 0) return -2;
        if (is_safe_for_removing(native_path_buffer, force) == 0) return VFS_ERROR;
        res = mount->fst->remove(mount->internal_index, native_path_buffer);
    }
    kfree(native_path_buffer);

    return res;
}

int vfs_chmod(struct vfs_struct * cwd, char* path, int mode) {return VFS_ERROR;}

void vfs_debug_by_path(struct vfs_struct * cwd, char* cpath) {
    if (cwd == 0 || cpath == 0) {
        panic("vfs_debug_by_path: Invalid arguments");
    }
    char * path = kmalloc(strlen(cpath) + 1);
    strcpy(path, cpath);
    vfs_normalize_path(cwd, path);
    vfs_print("vfs_debug_by_path(%s)\n", path);
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);
    if (mount != 0) {
        mount->fst->debug();
    }
    kfree(native_path_buffer);
}