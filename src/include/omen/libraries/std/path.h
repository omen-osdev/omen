#ifndef _PATH_H
#define _PATH_H

struct path {
    struct vfs_mount *mnt;
    struct dentry *dentry;
};

static inline int path_equal(const struct path *path1, const struct path *path2)
{
    return path1->mnt == path2->mnt && path1->dentry == path2->dentry;
}
#endif