#ifndef _PATH_H
#define _PATH_H

#include <omen/libraries/std/string.h>

struct path {
    struct vfs_mount *mnt;
    char * path;
};

static inline int path_equal(const struct path *path1, const struct path *path2)
{
    return path1->mnt == path2->mnt && !strcmp(path1->path, path2->path);
}
#endif