#include "utils.h"

#include <string.h>
#include <stdio.h>

void utils_path_split(const char *path, char *parent, char *child)
{
    const char *last_slash = strrchr(path, '/');

    if (!last_slash) {
        /* 슬래시 없음 → 루트 기준 */
        parent[0] = '/';
        parent[1] = '\0';
        strncpy(child, path, 511);
        child[511] = '\0';
        return;
    }

    if (last_slash == path) {
        /* "/test.txt" 처럼 루트 바로 아래 */
        parent[0] = '/';
        parent[1] = '\0';
        strncpy(child, path + 1, 511);
        child[511] = '\0';
        return;
    }

    /* "/foo/bar.txt" → parent="/foo", child="bar.txt" */
    int parent_len = (int)(last_slash - path);
    if (parent_len >= 512) parent_len = 511;
    strncpy(parent, path, parent_len);
    parent[parent_len] = '\0';

    strncpy(child, last_slash + 1, 511);
    child[511] = '\0';
}
