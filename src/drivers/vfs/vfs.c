#include "vfs.h"

#include <omen/libraries/allocators/heap_allocator.h>
#include <omen/apps/debug/debug.h>
#include <omen/libraries/std/string.h>
#include <omen/apps/panic/panic.h>

#include "partition/gpt.h"
#include "partition/mbr.h"

uint16_t vfs_root_size = 0;
struct vfs_devmap vfs_root[VFS_MAX_DEVICES];
struct vfs_file_system_type * file_system_type_list_head;
struct vfs_mount * mount_list_head;

struct vfs_mount * init_mount_header() {
    struct vfs_mount * mount = kmalloc(sizeof(struct vfs_mount));
    memset((void*)mount, 0, sizeof(struct vfs_mount));
    return mount;
}

struct vfs_partition * init_partition_header() {
    struct vfs_partition * part = kmalloc(sizeof(struct vfs_partition));
    memset((void*)part, 0, sizeof(struct vfs_partition));
    return part;
}

struct vfs_file_system_type * init_file_system_type_header() {
    struct vfs_file_system_type * fst = kmalloc(sizeof(struct vfs_file_system_type));
    memset((void*)fst, 0, sizeof(struct vfs_file_system_type));
    return fst;
}

uint8_t iterate_mounts(uint8_t (*callback)(struct vfs_mount*, void * data, void* passthrough), void * data, void* passthrough) {
    struct vfs_mount * mount = mount_list_head;
    while (mount != 0 && mount->device != 0 && mount->fst != 0 && mount->partition != 0) {
        if (callback(mount, data, passthrough)) return 1;
        mount = mount->next;
    }
    return 0;
}

void dump_mounts() {
    struct vfs_mount * mount = mount_list_head;
    while (mount != 0 && mount->device != 0 && mount->fst != 0 && mount->partition != 0) {
        kprintf("[%s] Dev: %s FS: %s II: %d\n", mount->partition->name, mount->device->name, mount->fst->name, mount->internal_index);
        mount = mount->next;
    }
}

void add_partition(struct vfs_partition * head, uint32_t lba, uint32_t size, uint8_t status, uint8_t type) {
    if (head == 0) {
        panic("[VFS] Invalid partition header");
    } else {
        struct vfs_partition * current = head;
        while (current->next != 0) {
            current = current->next;
        }
        current->next = init_partition_header();
        if (current->next == 0) panic("[VFS] Partition allocation failed!\n");
        current->lba = lba;
        current->size = size;
        current->status = status;
        current->type = type;
    }
}

void register_filesystem(struct vfs_compatible * registrar) {

    if (file_system_type_list_head == 0) file_system_type_list_head = init_file_system_type_header();
    
    struct vfs_file_system_type * fst = file_system_type_list_head;

    while(fst->next != 0) {
        if (!strcmp(fst->name, registrar->name))
            panic("[VFS] File system type with same name already registered\n");
        fst = fst->next;
    }
    
    for (int i = 0; i < registrar->major_no; i++) {
        fst->majors[i] = registrar->majors[i];
    }

    fst->major_no = registrar->major_no;
    fst->register_partition = registrar->register_partition;
    fst->unregister_partition = registrar->unregister_partition;
    fst->detect = registrar->detect;
    fst->flush = registrar->flush;
    
    fst->file_flush = registrar->file_flush;
    fst->file_open = registrar->file_open;
    fst->file_close = registrar->file_close;
    fst->file_creat = registrar->file_creat;
    fst->file_read = registrar->file_read;
    fst->file_write = registrar->file_write;
    fst->file_seek = registrar->file_seek;
    fst->file_tell = registrar->file_tell;
    fst->file_ioctl = registrar->file_ioctl;
    fst->file_stat = registrar->file_stat;
    fst->dir_open = registrar->dir_open;
    fst->dir_close = registrar->dir_close;
    fst->dir_read = registrar->dir_read;
    fst->dir_load = registrar->dir_load;
    fst->dir_creat = registrar->dir_creat;
    fst->prepare_remove = registrar->prepare_remove;
    fst->rename = registrar->rename;
    fst->remove = registrar->remove;
    fst->chmod = registrar->chmod;

    fst->debug = registrar->debug;

    //TODO: Expand to vfs_compat
    if (strlen(registrar->name) > VFS_COMPAT_FS_NAME_MAX_LEN) {
        kprintf("[VFS] Either FS name is too long or you are tryna hack us\n");
        kprintf("[VFS] anyway, im restricting it to %d chars\n", VFS_COMPAT_FS_NAME_MAX_LEN);
        strncpy(fst->name, registrar->name, VFS_COMPAT_FS_NAME_MAX_LEN);
    } else {
        strncpy(fst->name, registrar->name, strlen(registrar->name));
    }
    fst->next = init_file_system_type_header();
    if (fst->next == 0) panic("[VFS] File system type allocation failed!\n");
    kprintf("[VFS] Registered file system type %s\n", fst->name);
}

void add_mount(struct vfs_mount * head, struct device* device, struct vfs_partition * part, struct vfs_file_system_type* fst, int internal_index) {
    if (device == 0) panic("[VFS] Invalid device\n");
    if (part == 0) panic("[VFS] Invalid partition\n");
    if (fst == 0) panic("[VFS] Invalid fs\n");
    if (head == 0) {
        panic("[VFS] Invalid mount header");
    } else {
        struct vfs_mount * current = head;
        while (current->next != 0) {
            current = current->next;
        }
        current->next = init_mount_header();
        if (current->next == 0) panic("[VFS] Mount allocation failed!\n");
        
        current->device = device;
        current->partition = part;
        current->fst = fst;
        current->internal_index = internal_index;
    }
}

uint8_t mount_fs(struct device* dev, struct vfs_partition* partition, const char* mountpoint) {

    if (mount_list_head == 0) mount_list_head = init_mount_header();
    struct vfs_file_system_type * fst = file_system_type_list_head;
    if (fst == 0) {
        kprintf("[VFS] There are no registerd fs -\\_(-.-)_/-\n");
        return 0;
    }

    int fs_idx = 0;
    while (fst->next != 0) {
        int supported_major_number = fst->major_no;
        int found = 0;
        
        for (int i = 0; i < supported_major_number; i++) {
            if (((uint8_t)(fst->majors[i]) == ((uint8_t)(dev->bc << 7 | dev->major)))) {
                found = 1;
                break;
            }
        }
        if (!found) {
            fst = fst->next;
            fs_idx++;
            continue;
        }
        if (fst->detect(dev->name, partition->lba)) {
            kprintf("[VFS] Detected %s on %s, trying to mount on %s\n", fst->name, dev->name, mountpoint);
            int ret = fst->register_partition(dev->name, partition->lba, mountpoint);
            if (ret == -1) {
                kprintf("[VFS] Failed to mount %s on %s\n", fst->name, mountpoint);
                return 0;
            }
            snprintf(partition->name, 48, "%s", mountpoint);
            add_mount(mount_list_head, dev, partition, fst, ret);
            kprintf("[VFS] Mounted %s on %s index %d\n", fst->name, mountpoint, ret);
            return 1;
        }
        fst = fst->next;
        fs_idx++;
    }
    
    kprintf("[VFS] Device %s is not supported by any fs\n", dev->name);
    return 0;
}

uint32_t detect_partitions(struct device* dev, struct vfs_partition* part) {
    uint32_t partition_number = 0;
    uint8_t devmaj = (dev->bc << 7 | dev->major);
    if (devmaj < 8 || devmaj > 0xc) {
        partition_number = 1;
        add_partition(part, 0, 0, 0, 0);
    } else {
        if (partition_number == 0) {
            partition_number = read_gpt(dev->name, part, add_partition);
        }
        if (partition_number == 0) {
            partition_number = read_mbr(dev->name, part, add_partition);
        }
    }
    return partition_number;
}

void detect_devices() {
    vfs_root_size = get_device_count(); //TODO: Limit this to drives only!
    
    uint32_t device_index = 0;
    struct device* dev = get_first_device();
    
    while (dev != 0) {

        //Check if device is already registered
        for (uint32_t i = 0; i < vfs_root_size; i++) {
            if (vfs_root[i].dev == dev) {
                kprintf("[VFS] Device %s already registered\n", dev->name);
                dev = get_next_device(dev);
                continue;
            }
        }

        struct vfs_devmap* devmap;
        while(vfs_root[device_index].dev != 0) {
            device_index++;
        }
        devmap = &vfs_root[device_index];
        devmap->dev = dev;
        devmap->partitions = init_partition_header();
        kprintf("[VFS] Scanning device: %s\n", dev->name);

        uint32_t partitions = detect_partitions(dev, devmap->partitions);
        devmap->partition_no = partitions;
        if (partitions != 0 && devmap->partitions == 0)
            panic("[VFS] partitions detected but no partition struct found\n");
        if (partitions == 0)
            kprintf("[VFS] Skipping device %s, no partitions found\n", dev->name);
        struct vfs_partition* part = devmap->partitions;
        while (part->next != 0) {
            kprintf("[VFS] Partitions: %d, %d, %d, %d\n", part->lba, part->size, part->status, part->type);
            part = part->next;
        }

        dev = get_next_device(dev);
    }
}

char* get_full_path_from_fdentry(struct file_descriptor_entry* entry) {
    if (entry == 0) return 0;
    char * full_path = kmalloc(strlen(entry->name) + strlen(entry->mount) + 1);
    if (full_path == 0) return 0;
    strcpy(full_path, entry->mount);
    strcpy(full_path + strlen(entry->mount), entry->name);
    return full_path;
}

char* get_full_path_from_fd(int fd) {
    return get_full_path_from_fdentry(vfs_compat_get_file_descriptor(fd));  
}

char* get_full_path_from_dir(int fd) {
    return get_full_path_from_fdentry(&(vfs_compat_get_dir(fd)->fd));
}

int is_safe_for_removing(const char* path, uint8_t force) {
    if (is_open(path) > 0) {
        if (force == 0) {
            kprintf("[VFS] File is open, cannot remove\n");
            return 0;
        } else {
            kprintf("[VFS] File is open, but force is enabled, removing anyway\n");
            force_release(path);
        }
    }
    return 1;
}

struct vfs_mount* get_mount_from_path(const char* path, char* native_path) {
    struct vfs_mount * mount = mount_list_head;
    while (mount != 0 && mount->device != 0 && mount->fst != 0 && mount->partition != 0) {
        uint32_t mountpoint_len = strlen(mount->partition->name);
        if (strncmp(mount->partition->name, path, mountpoint_len) == 0) {
            memcpy(native_path, path + mountpoint_len, strlen(path) - mountpoint_len);
            native_path[strlen(path) - mountpoint_len] = 0;
            return mount;
        }
        mount = mount->next;
    }
    return 0;
}

void detect_partition_fs() {
    
    for (uint32_t i = 0; i < vfs_root_size; i++) {
        struct vfs_partition * current_partition = vfs_root[i].partitions;
        uint32_t partition_index = 0;
        char mountpoint[48];
        while(current_partition->next != 0) {
            //Check if partition is already mounted
            if (current_partition->name[0] != 0) {
                current_partition = current_partition->next;
                partition_index++;
                continue;
            }

            memset(mountpoint, 0, 48);
            snprintf(mountpoint, 48, "%sp%d", vfs_root[i].dev->name, partition_index);

            mount_fs(vfs_root[i].dev, current_partition, mountpoint);

            current_partition = current_partition->next;
            partition_index++;
        }
    }
}

void probe_fs() {
    kprintf("[VFS] Probing FS\n");
    detect_devices();
    detect_partition_fs();
}

void init_vfs() {
    kprintf("### VFS STARTUP ###\n");
    detect_devices();
    detect_partition_fs();
    dump_mounts();
    kprintf("### VFS STARTUP END ###\n");
}