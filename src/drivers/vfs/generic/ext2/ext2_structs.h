#ifndef _EXT2_STRUCTS_H
#define _EXT2_STRUCTS_H

struct ext2_block_group_descriptor {
    uint32_t bg_block_bitmap;        /* Blocks bitmap block */
    uint32_t bg_inode_bitmap;       /* Inodes bitmap block */
    uint32_t bg_inode_table;        /* Inodes table block */
    uint16_t bg_free_blocks_count;  /* Free blocks count */
    uint16_t bg_free_inodes_count;  /* Free inodes count */
    uint16_t bg_used_dirs_count;    /* Directories count */
    uint8_t  bg_pad[14];            /* Padding to the end of the block */
} __attribute__((packed));

struct ext2_directory_entry {
    uint32_t inode;                 /* Inode number */
    uint16_t rec_len;               /* Directory entry length */
    uint8_t  name_len;              /* Name length */
    uint8_t  file_type;             /* File type */
    char     name[255];   /* File name */ //TODO: CONVERT THIS TO A POINTER
} __attribute__((packed));

struct ext2_inode_descriptor_generic {
    uint16_t i_mode;                /* File mode */
    uint16_t i_uid;                 /* Low 16 bits of Owner Uid */
    uint32_t i_size;                /* Size in bytes */
    uint32_t i_atime;               /* Access time */
    uint32_t i_ctime;               /* Creation time */
    uint32_t i_mtime;               /* Modification time */
    uint32_t i_dtime;               /* Deletion Time */
    uint16_t i_gid;                 /* Low 16 bits of Group Id */
    uint16_t i_links_count;         /* Links count */
    uint32_t i_sectors;             /* sector count */
    uint32_t i_flags;               /* File flags */
    uint32_t i_osd1;                /* OS dependent 1 */
    uint32_t i_block[15];           /* Pointers to blocks */
    uint32_t i_generation;          /* File version (for NFS) */
    uint32_t i_file_acl;            /* File ACL */
    uint32_t i_dir_acl;             /* Directory ACL */
    uint32_t i_faddr;               /* Fragment address */
} __attribute__((packed));

struct ext2_inode_descriptor {
    struct ext2_inode_descriptor_generic id;
    uint8_t  i_osd2[12];            /* OS dependent 2 */
} __attribute__((packed));

struct ext2_inode_descriptor_linux {
    struct   ext2_inode_descriptor_generic id;
    uint8_t  i_fragment_no;          /* Fragment number */
    uint8_t  i_fragment_size;        /* Fragment size */
    uint16_t i_osd2;                /* OS dependent 2 */
    uint16_t i_high_uid;            /* High 16 bits of Owner Uid */
    uint16_t i_high_gid;            /* High 16 bits of Group Id */
    uint32_t i_reserved;
} __attribute__((packed));

struct ext2_inode_descriptor_hurd {
    struct   ext2_inode_descriptor_generic id;
    uint8_t  i_frag;               /* Fragment number */
    uint8_t  i_fsize;               /* Fragment size */
    uint16_t i_high_type_perm;      /* High 16 bits of Type and Permission */
    uint16_t i_high_uid;            /* High 16 bits of Owner Uid */
    uint16_t i_high_gid;            /* High 16 bits of Group Id */
    uint32_t i_author_id;           /* Author ID */
} __attribute__((packed));

struct ext2_inode_descriptor_masix {
    struct  ext2_inode_descriptor_generic id;
    uint8_t i_frag;                 /* Fragment number */
    uint8_t i_fsize;                /* Fragment size */
    uint8_t i_reserved[10];         /* Reserved */    
} __attribute__((packed));

struct ext2_partition {
    char name[32];
    char disk[32];
    int32_t backup_bgs[64];
    uint32_t backup_bgs_count;
    uint32_t lba;
    uint32_t group_number;
    uint32_t sector_size;
    uint32_t bgdt_block;
    uint32_t sb_block;
    uint8_t flush_required;
    struct ext2_superblock_extended *sb;
    struct ext2_block_group_descriptor *gd;
    struct ext2_partition *next;
};

struct ext2_superblock {
    uint32_t s_inodes_count;        /* Inodes count */
    uint32_t s_blocks_count;        /* Blocks count */
    uint32_t s_r_blocks_count;      /* Reserved blocks count */
    uint32_t s_free_blocks_count;   /* Free blocks count */
    uint32_t s_free_inodes_count;   /* Free inodes count */
    uint32_t s_first_sb_block;      /* First superblock Block */
    uint32_t s_log_block_size;      /* Block size */
    uint32_t s_log_frag_size;       /* Fragment size */
    uint32_t s_blocks_per_group;    /* # Blocks per group */
    uint32_t s_frags_per_group;     /* # Fragments per group */
    uint32_t s_inodes_per_group;    /* # Inodes per group */
    uint32_t s_mtime;               /* Mount time */
    uint32_t s_wtime;               /* Write time */
    uint16_t s_mnt_count;           /* Mount count */
    uint16_t s_max_mnt_count;       /* Maximal mount count */
    uint16_t s_magic;               /* Magic signature */
    uint16_t s_state;               /* File system state */
    uint16_t s_errors;              /* Behaviour when detecting errors */
    uint16_t s_minor_rev_level;     /* minor revision level */
    uint32_t s_lastcheck;           /* time of last check */
    uint32_t s_checkinterval;       /* max. time between checks */
    uint32_t s_creator_os;          /* OS */
    uint32_t s_rev_level;           /* Revision level */
    uint16_t s_def_resuid;          /* Default uid for reserved blocks */
    uint16_t s_def_resgid;          /* Default gid for reserved blocks */
} __attribute__((packed));

struct ext2_superblock_extended {
    struct ext2_superblock sb;
    uint32_t s_first_ino;           /* First non-reserved inode */
    uint16_t s_inode_size;          /* size of inode structure */
    uint16_t s_block_group_nr;      /* block group # of this superblock */
    uint32_t s_feature_compat;      /* compatible feature set */
    uint32_t s_feature_incompat;    /* incompatible feature set */
    uint32_t s_feature_ro_compat;   /* readonly-compatible feature set */
    uint8_t  s_uuid[16];            /* 128-bit uuid for volume */
    char     s_volume_name[16];     /* volume name */
    char     s_last_mounted[64];    /* directory where last mounted */
    uint32_t s_algo_bitmap;         /* For compression */
    uint8_t  s_prealloc_blocks;     /* Nr of blocks to try to preallocate*/
    uint8_t  s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
    uint16_t s_padding1;
    uint8_t  s_journal_uuid[16];    /* uuid of journal superblock */
    uint32_t s_journal_inum;        /* inode number of journal file */
    uint32_t s_journal_dev;         /* device number of journal file */
    uint32_t s_last_orphan;         /* start of list of inodes to delete */
    uint8_t  s_unused[788];         /* Padding to the end of the block */
} __attribute__((packed));

#endif
