cmake_minimum_required(VERSION 3.16)
project(uffs-crash-consistency C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# FUSE3 패키지 탐색
find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE3 REQUIRED fuse3)

add_executable(uffs
    src/main.c
    src/flash.c
    src/tree.c
    src/gc.c
    src/file.c
    src/dir.c
    src/meta.c
    src/utils.c
)

target_include_directories(uffs PRIVATE
    src
    ${FUSE3_INCLUDE_DIRS}
)

target_compile_options(uffs PRIVATE
    ${FUSE3_CFLAGS_OTHER}
    -Wall
    -Wextra
    -Werror
)

target_link_libraries(uffs PRIVATE
    ${FUSE3_LIBRARIES}
    pthread
)

enable_testing()
add_subdirectory(tests)
