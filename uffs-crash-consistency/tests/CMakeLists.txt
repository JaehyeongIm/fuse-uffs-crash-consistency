cmake_minimum_required(VERSION 3.16)

# ─── Google Test ─────────────────────────────────────────────
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(STATUS "GTest 없음: test_integration 빌드 건너뜀")
    message(STATUS "  Ubuntu: sudo apt-get install libgtest-dev")
else()
    include(GoogleTest)

    add_executable(test_integration test_integration.cpp)

    target_link_libraries(test_integration PRIVATE
        GTest::gtest
        GTest::gtest_main
    )

    target_compile_options(test_integration PRIVATE
        -Wall -Wextra
    )

    # CTest 등록 (UFFS_MOUNT_PATH 는 실행 시 환경변수로 제공)
    gtest_discover_tests(test_integration
        PROPERTIES ENVIRONMENT "UFFS_MOUNT_PATH=$ENV{UFFS_MOUNT_PATH}"
    )

    message(STATUS "test_integration 빌드 대상 등록됨")
    message(STATUS "  실행: UFFS_MOUNT_PATH=/mnt/uffs ./test_integration")
endif()

# ─── 쉘 스크립트: 빌드 디렉토리에 복사 ─────────────────────
foreach(SCRIPT
    tc_crash_005.sh
    tc_crash_dir_001.sh
    tc_crash_neg_001.sh
    tc_rel_001.sh
)
    configure_file(${SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT} COPYONLY)
    # 실행 권한 부여
    file(CHMOD ${CMAKE_CURRENT_BINARY_DIR}/${SCRIPT}
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                     GROUP_READ GROUP_EXECUTE
                     WORLD_READ WORLD_EXECUTE)
endforeach()

message(STATUS "쉘 스크립트 복사됨: build/tests/tc_crash_*.sh, tc_rel_001.sh")
