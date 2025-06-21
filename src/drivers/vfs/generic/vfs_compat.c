#include "vfs_compat.h"
#include "../vfs.h"
#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/std/string.h>

dir_t entries_table[VFS_COMPAT_MAX_OPEN_FILES] = {0};

#define GET_DIR(fd) ((dir_t*)(&(entries_table[fd])))
#define GET_FILE(desc) ((struct file_descriptor_entry*)(&(GET_DIR(desc)->fd)))

dir_t * vfs_compat_get_dir(int fd) {
    if (fd >= VFS_COMPAT_MAX_OPEN_FILES) return 0;
    dir_t * entry = GET_DIR(fd);
    return entry;
}

struct file_descriptor_entry * vfs_compat_get_file_descriptor(int fd) {
    if (fd >= VFS_COMPAT_MAX_OPEN_FILES) return 0;
    struct file_descriptor_entry * entry = GET_FILE(fd);
    return entry;
}

int get_fd(const char* path, const char* mount, int flags, int mode) {
    if (strlen(path) >  VFS_FDE_NAME_MAX_LEN) return VFS_ERROR;
    if (strlen(mount) > VFS_FDE_NAME_MAX_LEN) return VFS_ERROR; 
    static int fd = 0;
    for (int i = 0; i < VFS_COMPAT_MAX_OPEN_FILES; i++) {
        struct file_descriptor_entry * entry = GET_FILE(fd);
        if (entry->loaded == 0) {
            entry->loaded = 1;
            entry->flags = flags;
            entry->mode = mode;
            entry->offset = 0;
            strncpy(entry->name, path, strlen(path));
            strncpy(entry->mount, mount, strlen(mount));

            return fd++;
        } else {
            fd++;
            if (fd >= VFS_COMPAT_MAX_OPEN_FILES) fd = 0;
        }
    }
    return VFS_ERROR;
}

int get_dirfd(const char* path, const char* mount, int flags, int mode) {
    if (strlen(path) >  VFS_FDE_NAME_MAX_LEN) return VFS_ERROR;
    if (strlen(mount) > VFS_FDE_NAME_MAX_LEN) return VFS_ERROR; 
    static int fd = 0;
    for (int i = 0; i < VFS_COMPAT_MAX_OPEN_FILES; i++) {
        dir_t * entry = GET_DIR(fd);
        if (entry->fd.loaded == 0) {
            entry->fd.loaded = 1;
            entry->fd.flags = flags;
            entry->fd.mode = mode;
            entry->fd.offset = 0;
            strncpy(entry->fd.name, path, strlen(path));
            strncpy(entry->fd.mount, mount, strlen(mount));
            
            entry->index = 0;
            entry->number = 0;
            entry->dentries = (struct dentry*) kmalloc(sizeof(struct dentry));
            memset(entry->dentries, 0, sizeof(struct dentry));

            return fd++;
        } else {
            fd++;
            if (fd >= VFS_COMPAT_MAX_OPEN_FILES) fd = 0;
        }
    }
    return VFS_ERROR;
}

int dup2_fd(int oldfd, int newfd) {
    if (oldfd >= VFS_COMPAT_MAX_OPEN_FILES) return VFS_ERROR;
    if (newfd >= VFS_COMPAT_MAX_OPEN_FILES) return VFS_ERROR;
    struct file_descriptor_entry * old_file = GET_FILE(oldfd);
    struct file_descriptor_entry * new_file = GET_FILE(newfd);
    if (old_file->loaded == 0) return VFS_ERROR;
    if (new_file->loaded == 1) {
        new_file->loaded = 0;
    }
    new_file->loaded = 1;
    new_file->flags = old_file->flags;
    new_file->mode = old_file->mode;
    new_file->offset = old_file->offset;
    strncpy(new_file->name, old_file->name, strlen(old_file->name));
    strncpy(new_file->mount, old_file->mount, strlen(old_file->mount));
    return newfd;
}

int dup_fd(int oldfd, int newfd) {
    //if newfd != VFS_ERROR then apply dup2's logic
    if (newfd != -1) return dup2_fd(oldfd, newfd);
    //else behave like dup and ignore newfd
    if (oldfd >= VFS_COMPAT_MAX_OPEN_FILES) return VFS_ERROR;
    struct file_descriptor_entry * old_file = GET_FILE(oldfd);
    if (old_file->loaded == 0) return VFS_ERROR;

    for (int i = 0; i < VFS_COMPAT_MAX_OPEN_FILES; i++) {
        struct file_descriptor_entry * entry = GET_FILE(i);
        if (entry->loaded == 0) {
            entry->loaded = 1;
            entry->flags = old_file->flags;
            entry->mode = old_file->mode;
            entry->offset = old_file->offset;
            strncpy(entry->name, old_file->name, strlen(old_file->name));
            strncpy(entry->mount, old_file->mount, strlen(old_file->mount));
            return i;
        }
    }
    return VFS_ERROR;
}

int is_open(const char* path) {
    for (int i = 0; i < VFS_COMPAT_MAX_OPEN_FILES; i++) {
        struct file_descriptor_entry * entry = GET_FILE(i);
        if (entry->loaded == 1) {
            if (strcmp(entry->name, path) == 0) {
                return i;
            }
        }
    }
    return VFS_ERROR;
}

int force_release(const char * path) {
    int changes = 0;
    for (int i = 0; i < VFS_COMPAT_MAX_OPEN_FILES; i++) {
        struct file_descriptor_entry * entry = GET_FILE(i);
        if (entry->loaded == 1) {
            if (strcmp(entry->name, path) == 0) {
                entry->loaded = 0;
                changes++;
            }
        }
    }
    return changes;
}

int release_fd(int fd) {
    if (fd >= VFS_COMPAT_MAX_OPEN_FILES) return VFS_ERROR;
    GET_FILE(fd)->loaded = 0;
    return 0;
}

int read_dirfd(int fd, char * name, uint32_t * name_len, uint32_t * type) {
    if (fd >= VFS_COMPAT_MAX_OPEN_FILES) return VFS_ERROR;
    dir_t * entry = GET_DIR(fd);
    if (entry->fd.loaded == 0) return VFS_ERROR;
    if (entry->index >= entry->number) return 0;

    uint32_t index = entry->index;
    struct dentry * dentry = entry->dentries;
    if (dentry == 0 || dentry->next == 0) return 0;
    dentry = dentry->next; // Skip the first entry as it is empty
    for (uint32_t i = 0; i < index; i++) {
        //DBG_DEBUG("dentry: %s, nl: %d, t: %d\n", dentry->name, dentry->name_len, dentry->type);
        dentry = dentry->next;
    }
    strncpy(name, dentry->name, dentry->name_len);
    *name_len = dentry->name_len;
    *type = dentry->type;
    entry->index++;
    return 1;
}

int seek_dirfd(int fd, int offset, int whence) {
    if (fd >= VFS_COMPAT_MAX_OPEN_FILES) return VFS_ERROR;
    dir_t * entry = GET_DIR(fd);
    if (entry->fd.loaded == 0) return VFS_ERROR;
    if (whence == SEEK_SET) {
        if (offset < 0 || offset >= entry->number) return VFS_ERROR;
        entry->index = offset;
    } else if (whence == SEEK_CUR) {
        if (offset < 0 && entry->index + offset < 0) return VFS_ERROR;
        if (offset > 0 && entry->index + offset >= entry->number) return VFS_ERROR;
        entry->index += offset;
    } else if (whence == SEEK_END) {
        if (offset > 0 || entry->number + offset < 0) return VFS_ERROR;
        entry->index = entry->number + offset;
    } else {
        return VFS_ERROR; // Invalid whence
    }
    if (entry->index < 0) entry->index = 0;
    if (entry->index >= entry->number) entry->index = entry->number - 1;
    //DBG_DEBUG("seek_dirfd: %d, %d, %d\n", fd, offset, whence);
    //DBG_DEBUG("New index: %d\n", entry->index);
    return 1;
}

int tell_dirfd(int fd) {
    if (fd >= VFS_COMPAT_MAX_OPEN_FILES) return VFS_ERROR;
    dir_t * entry = GET_DIR(fd);
    if (entry->fd.loaded == 0) return VFS_ERROR;
    return entry->index;
}

int release_dirfd(int fd) {
    if (fd >= VFS_COMPAT_MAX_OPEN_FILES) return VFS_ERROR;
    dir_t * entry = GET_DIR(fd);
    entry->fd.loaded = 0;
    struct dentry * dentry_head = entry->dentries;
    struct dentry * dentry = dentry_head;
    while (dentry->next != 0) {
        dentry = dentry->next;
        kfree(dentry);
    }
    return 0;
}

uint8_t add_file_to_dirfd(int fd, const char* name, uint32_t inode, uint32_t type, uint32_t name_len) {
    //DBG_DEBUG("add_file_to_dirfd: %d, %s, %d, %d, %d\n", fd, name, inode, type, name_len);
    if (fd >= VFS_COMPAT_MAX_OPEN_FILES) return 0;
    if (strlen(name) > VFS_FDE_NAME_MAX_LEN) return 0;
    dir_t * entry = GET_DIR(fd);
    if (entry->fd.loaded == 0) return 0;
    struct dentry * dentry_head = entry->dentries;
    struct dentry * dentry = dentry_head;
    while (dentry->next != 0) {
        dentry = dentry->next;
    }
    dentry->next = (struct dentry*) kmalloc(sizeof(struct dentry));
    dentry = dentry->next;
    dentry->next = 0;
    dentry->inode = inode;
    dentry->type = type;
    dentry->name_len = name_len;
    strncpy(dentry->name, name, strlen(name));
    entry->number++;
    //DBG_DEBUG("added file to dirfd: %s new number\n", name, open_directory_table[fd].number);
    
    return 1;
}