#pragma once

/*
 * utils_path_split: 경로를 부모 경로와 파일/디렉토리 이름으로 분리
 *   "/foo/bar.txt" → parent="/foo",  child="bar.txt"
 *   "/test.txt"    → parent="/",     child="test.txt"
 *   "/"            → parent="",      child=""
 *
 * parent, child 버퍼는 최소 512바이트 이상으로 제공해야 한다.
 */
void utils_path_split(const char *path, char *parent, char *child);
