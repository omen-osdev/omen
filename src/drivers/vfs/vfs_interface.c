#include "vfs_interface.h"
#include "vfs.h"
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/std/string.h>

#define PRINT_ENABLE 0
#define vfs_print(...) if (PRINT_ENABLE) kprintf(__VA_ARGS__)

void vfs_normalize_path(char * path) {
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

    //Remove trailing slashes and dots
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

int vfs_file_open(char* path, int flags, int mode) {
    char * npath = kmalloc(strlen(path) + 1);
    strcpy(npath, path);
    vfs_normalize_path(npath);
    vfs_print("vfs_file_open(%s, %d, %d)\n", npath, flags, mode);
    char * native_path_buffer = kmalloc(strlen(npath) + 1);
    struct vfs_mount* mount = get_mount_from_path(npath, native_path_buffer);
    int res = -1;
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
        return -1;
    }

    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);
    int res = -1;
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
        return -1;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = -1;
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
        return -1;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = -1;
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
        return -1;
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

int vfs_file_creat(char* path, int mode) {
    char * npath = kmalloc(strlen(path) + 1);
    strcpy(npath, path);
    vfs_normalize_path(npath);
    vfs_print("vfs_file_creat(%s, %d)\n", npath, mode);
    char * native_path_buffer = kmalloc(strlen(npath) + 1);
    struct vfs_mount* mount = get_mount_from_path(npath, native_path_buffer);
    int res = -1;
    if (mount != 0) {
        res = mount->fst->file_creat(mount->internal_index, native_path_buffer, mode);
    }
    kfree(native_path_buffer);
    kfree(npath);
    return res;
}

int vfs_file_stat(int fd, stat_t* st) {
    vfs_print("vfs_file_stat(%d, %p)\n", fd, st);
    char * path = get_full_path_from_fd(fd);
    if (path == 0) {
        return -1;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = -1;
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
        return -1;
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
        return -1;
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

int vfs_dir_open(char* path) {
    char * npath = kmalloc(strlen(path) + 1);
    strcpy(npath, path);
    vfs_normalize_path(npath);
    vfs_print("vfs_dir_open(%s)\n", path);
    char * native_path_buffer = kmalloc(strlen(npath) + 1);
    struct vfs_mount* mount = get_mount_from_path(npath, native_path_buffer);
    int res = -1;
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
        return -1;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = -1;
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
        return -1;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = -1;
    if (mount != 0) {
        res = mount->fst->dir_load(mount->internal_index, fd);
    }

    kfree(native_path_buffer);
    kfree(path);
    return res;
}

void vfs_dir_list(char* path) {
    char * npath = kmalloc(strlen(path) + 1);
    strcpy(npath, path);
    vfs_normalize_path(npath);
    int fd = vfs_dir_open(npath);
    if (fd < 0) {
        kprintf("Error opening directory %s\n", npath);
        return;
    }

    int res = vfs_dir_load(fd);
    if (res < 0) {
        kprintf("Error loading directory %s\n", npath);
        kfree(npath);
        vfs_dir_close(fd);
        return;
    }

    kprintf("Directory %s contents:\n", npath);
    char name_buffer[256];
    uint32_t type;
    uint32_t name_len; 
    while (vfs_dir_read(fd, name_buffer, &name_len, &type) > 0) {
        kprintf("DIR ENTRY: %s, %d, %d\n", name_buffer, type, name_len);
    }

    vfs_dir_close(fd);
    kfree(npath);
    return;
}

int vfs_file_search(const char * name, char * cpath) {
    if (cpath == 0 || name == 0) {
        return -1;
    }
    char * path = kmalloc(strlen(cpath) + 1);
    strcpy(path, cpath);
    vfs_normalize_path(path);
    char * new_path = kmalloc(1024);
    memset(new_path, 0, 1024);
    strcpy(new_path, path);
    strcat(new_path, ".");
    
    int fd = vfs_dir_open(new_path);
    if (fd < 0) {
        kprintf("Error opening directory %s\n", new_path);
        return;
    }

    int res = vfs_dir_load(fd);
    if (res < 0) {
        kprintf("Error loading directory %s\n", new_path);
        return;
    }

    char name_buffer[256];
    uint32_t type;
    uint32_t name_len;
    while (vfs_dir_read(fd, name_buffer, &name_len, &type) > 0) {
        //kprintf("Searching %s Type %d\n", name_buffer, type);
        if (strcmp(name_buffer, name) == 0) {
            kprintf("Found %s\n", name_buffer);
            vfs_dir_close(fd);
            memset(path, 0, 1024);
            strcpy(path, new_path);
            //Swap the last dot with a slash
            path[strlen(path) - 1] = '/';
            strcat(path, name_buffer);
            kfree(new_path);
            return 1;
        }
        if (type == 0x2 && strcmp(name_buffer, ".") != 0 && strcmp(name_buffer, "..") != 0 && strcmp(name_buffer, "lost+found") != 0) {
            //kprintf("Nesting into %s\n", name_buffer);
            memset(new_path, 0, 1024);
            strcpy(new_path, path);
            if (new_path[strlen(new_path) - 1] != '/') {
                strcat(new_path, "/");
            }
            strcat(new_path, name_buffer);
            int res = vfs_file_search(name, new_path);
            if (res == 1) {
                memset(path, 0, 1024);
                strcpy(path, new_path);
                vfs_dir_close(fd);
                kfree(new_path);
                return 1;
            }
        }
    }

    vfs_dir_close(fd);
    kfree(new_path);
    return 0;
}

int vfs_dir_read(int fd, char* name, uint32_t * name_len, uint32_t * type) {
    vfs_print("vfs_dir_read(%d)\n", fd);

    char * path = get_full_path_from_dir(fd);
    if (path == 0) {
        return -1;
    }
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = -1;
    if (mount != 0) {
        res = mount->fst->dir_read(mount->internal_index, fd, name, name_len, type);
    }

    kfree(native_path_buffer);
    kfree(path);
    return res;
}

int vfs_mkdir(char* cpath, int mode) {
    char * path = kmalloc(strlen(cpath) + 1);
    strcpy(path, cpath);
    vfs_normalize_path(path);
    vfs_print("vfs_dir_creat(%s)\n", path);

    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);
    int res = -1;
    if (mount != 0) {
        res = mount->fst->dir_creat(mount->internal_index, native_path_buffer, mode);
    }
    kfree(native_path_buffer);
    return res;
}

int vfs_rename(char* path, const char* name) {return -1;}

int vfs_remove(char* cpath, uint8_t force) {
    char * path = kmalloc(strlen(cpath) + 1);
    strcpy(path, cpath);
    vfs_normalize_path(path);
    vfs_print("vfs_remove(%s)\n", path);
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);

    int res = -1;
    if (mount != 0) {
        res = mount->fst->prepare_remove(mount->internal_index, native_path_buffer);
        if (res == 0) return -2;
        if (is_safe_for_removing(native_path_buffer, force) == 0) return -1;
        res = mount->fst->remove(mount->internal_index, native_path_buffer);
    }
    kfree(native_path_buffer);

    return res;
}

int vfs_chmod(char* path, int mode) {return -1;}

void vfs_debug_by_path(char* cpath) {
    char * path = kmalloc(strlen(cpath) + 1);
    strcpy(path, cpath);
    vfs_normalize_path(path);
    vfs_print("vfs_debug_by_path(%s)\n", path);
    char * native_path_buffer = kmalloc(strlen(path) + 1);
    struct vfs_mount* mount = get_mount_from_path(path, native_path_buffer);
    if (mount != 0) {
        mount->fst->debug();
    }
    kfree(native_path_buffer);
}