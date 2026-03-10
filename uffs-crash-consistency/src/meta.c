#include "meta.h"
#include "tree.h"

#include <string.h>
#include <errno.h>

int meta_getattr(const char *path, struct stat *stbuf,
                 struct fuse_file_info *fi)
{
    (void)fi;
    memset(stbuf, 0, sizeof(*stbuf));

    /* 루트 디렉토리 */
    if (strcmp(path, "/") == 0) {
        TreeNode *root = tree_find_dir("/");
        if (!root) return -ENOENT;
        stbuf->st_mode  = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size  = 0;
        stbuf->st_atime = (time_t)root->last_modify;
        stbuf->st_mtime = (time_t)root->last_modify;
        stbuf->st_ctime = (time_t)root->create_time;
        return 0;
    }

    /* 디렉토리 검색 */
    TreeNode *dir = tree_find_dir(path);
    if (dir) {
        stbuf->st_mode  = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_size  = 0;
        stbuf->st_atime = (time_t)dir->last_modify;
        stbuf->st_mtime = (time_t)dir->last_modify;
        stbuf->st_ctime = (time_t)dir->create_time;
        return 0;
    }

    /* 파일 검색 */
    TreeNode *file = tree_find_file(path);
    if (file) {
        stbuf->st_mode  = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size  = (off_t)file->size;
        stbuf->st_atime = (time_t)file->last_modify;
        stbuf->st_mtime = (time_t)file->last_modify;
        stbuf->st_ctime = (time_t)file->create_time;
        return 0;
    }

    return -ENOENT;
}
