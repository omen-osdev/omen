/**
 * @file types.h
 * @brief Types and structures for the virtual file system.
 * @authors OMEN Team, aziabatz
 */

#ifndef OMEN_BASE_VFS_TYPES
#define OMEN_BASE_VFS_TYPES

#include <omen/libraries/std/stdint.h>
#include <omen/managers/dev/devices.h>

/**
 * @brief File system timestamps.
 */
typedef struct fs_times
{
    // FIXME: someone define unix time type elsewhere
    uint64_t creation; /**< File creation time. */
    uint64_t modification; /**< Last modification time. */
    uint64_t write; /**< Last write time. */
} fs_times_t;

typedef struct vfs_fd {
    uint32_t flags; /**< Descriptor flags. */
    char * pathname; /**< File pathname. */

} file_descriptor_t;

/**
 * @brief VFS file operations.
 */
typedef struct vfs_file_op {
    /**
     * @brief Write data to a file.
     * @param 1 File descriptor.
     * @param 2 Buffer to write.
     * @param 3 Size of data to write.
     * @return Number of bytes written.
     */
    ssize_t (*write) (file_descriptor_t *, const void*, size_t);
    /**
     * @brief Read data from a file.
     * @param 1 File descriptor.
     * @param 2 Buffer to read into.
     * @param 3 Size of data to read.
     * @return Number of bytes read.
     */
    size_t (*read) (file_descriptor_t *,void*, size_t);
    /**
     * @brief Seek to a position in a file.
     * @param 1 File descriptor.
     * @param 2 Offset to seek to.
     * @param 3 Seek mode.
     * @return New offset.
     */
    off_t (*seek) (file_descriptor_t *, uint64_t, int);
    /**
     * @brief Perform an I/O control operation.
     * @param 1 File descriptor.
     * @param 2 Control command.
     * @param 3 Data for command.
     * @return Result of the operation.
     */
    uint64_t (*ioctl) (file_descriptor_t*,  uint64_t, void *);
    /**
     * @brief Open a file.
     * @param 1 File descriptor.
     * @return Status of the open operation.
     */
    uint64_t (*open) (file_descriptor_t*);
    /**
     * @brief Close a file.
     * @param 1 File descriptor.
     * @return Status of the close operation.
     */
    uint64_t (*close) (file_descriptor_t*);
    /**
     * @brief Read directory entries.
     * @param 1 Directory file descriptor.
     * @param 2 Buffer to store entries.
     * @param 3 Size of buffer.
     * @return Number of entries read.
     */
    uint64_t (*readdir) (file_descriptor_t*, char*, size_t);
} file_op_t;

// TODO: Keep this if we need it for FS that aren't based on file abstraction
typedef file_op_t inode_op_t;

typedef struct vfs_inode {
    uint64_t inode; /**< Inode number. */
    size_t size; /**< File size. */
    uint16_t permission; /**< File permissions. */
    uint32_t flags; /**< Inode flags. */
    fs_times_t * time; /**< Timestamps. */
    uint64_t pointed_count; /**< Reference count. */
    size_t blocksize; /**< Block size. */
    uint8_t locked; /**< Lock state. */
    uint8_t touched; /**< Modification flag. */
 
    union operations
    {
        inode_op_t * inode_operations; /**< Inode operations. */
        file_op_t * file_operations; /**< File operations. */
    };
    
    void * fs_extra; /**< Filesystem-specific data. */
} inode_t;

typedef struct vfs_fs_operations {
    /**
     * @brief Allocate an inode.
     * @param 1 Pointer to inode.
     */
    void (*alloc_inode) (inode_t *);
    /**
     * @brief Free an inode.
     * @param 1 Pointer to inode.
     */
    void (*free_inode) (inode_t *);

    /**
     * @brief Read an inode from disk.
     * @param 1 Pointer to inode.
     */
    void (*read_inode) (inode_t *);
    /**
     * @brief Write an inode to disk.
     * @param 1 Pointer to inode.
     * @param 2 Whether to sync changes immediately.
     */
    void (*write_inode) (inode_t *, int sync);

    /**
     * @brief Release an inode.
     * @param 1 Pointer to inode.
     * @return Status of the release operation.
     */
    int (*release_inode) (inode_t *);
    /**
     * @brief Lock an inode.
     * @param 1 Pointer to inode.
     * @return Status of the lock operation.
     */
    int (*lock_inode) (inode_t*);
    
    /**
     * @brief Delete an inode.
     * @param 1 Pointer to inode.
     */
    void (*delete_inode) (inode_t *);

    /**
     * @brief Release filesystem.
     * @param 1 Filesystem descriptor.
     */
    void (*release_fs) (fs_descriptor_t *);
    /**
     * @brief Modify filesystem descriptor.
     * @param 1 Pointer to new filesystem descriptor.
     */
    void (*write_fs) (fs_descriptor_t *);

    /**
     * @brief Synchronize filesystem.
     * @param 1 Filesystem descriptor.
     * @param sync Whether to do it as blocking operation
     * @return Status of the sync operation.
     */
    int (*sync_fs) (fs_descriptor_t *, int sync);

    /**
     * @brief Lock the filesystem.
     * @param 1 Filesystem descriptor.
     */
    void (*lock_fs) (fs_descriptor_t *);
    /**
     * @brief Unlock the filesystem.
     * @param 1 Filesystem descriptor.
     */
    void (*unlockfs) (fs_descriptor_t *);

    /**
     * @brief Mount the filesystem.
     * @param 1 Filesystem descriptor.
     * @param 2 Mount flags.
     * @return Status of the mount operation.
     */
    int (*mount_fs) (fs_descriptor_t *, int *);
    /**
     * @brief Unmount the filesystem.
     * @param 1 Filesystem descriptor.
     */
    void (*unmount_fs) (fs_descriptor_t *);
}fs_op_t;

// See 
typedef struct vfs_fs_descriptor {
    device_t * fs_device; /**< Filesystem device. */ 
    inode_t * root; /**< Root inode. */ 
    size_t blocksize; /**< Block size. */ 

    size_t inode_count; /**< Total inodes. */ 
    size_t block_count; /**< Total blocks. */ 

    size_t unused_inode_count; /**< Free inodes. */ 
    size_t unused_block_count; /**< Free blocks. */ 

    fs_op_t * filesystem_operations; /**< Filesystem operations. */ 
    char * fs_type; /**< Filesystem type. */ 
    void * fs_extra; /**< Filesystem-specific data. */ 
} fs_descriptor_t;

#endif