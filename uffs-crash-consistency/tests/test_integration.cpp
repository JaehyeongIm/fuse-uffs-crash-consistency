/**
 * TC-FILE-001~007, TC-DIR-002~004, TC-META-001 통합 테스트
 *
 * 실행 전제:
 *   uffs 파일시스템이 마운트되어 있어야 한다.
 *   환경변수 UFFS_MOUNT_PATH 에 마운트 경로를 설정한다.
 *
 * 실행 방법:
 *   UFFS_MOUNT_PATH=/mnt/uffs ./test_integration
 *
 * 판정 원칙 (VER-ORACLE-001):
 *   내부 플래시 블록이 아닌 FUSE 인터페이스 (read/stat/readdir) 로 결과를 판정한다.
 */

#include <gtest/gtest.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include <string>
#include <vector>

/* ─── 전역 마운트 경로 ───────────────────────────────────── */
static const char *g_mount = nullptr;

/* 테스트 파일 크기 (환경변수 UFFS_TEST_FILE_SIZE 로 재정의 가능) */
static size_t g_file_size = 64 * 1024; /* 64KB 기본값 */

/* ─── 테스트 픽스처 ──────────────────────────────────────── */

class UffsTest : public ::testing::Test {
protected:
    std::string test_dir_;

    void SetUp() override {
        ASSERT_NE(g_mount, nullptr)
            << "UFFS_MOUNT_PATH 환경변수가 설정되지 않았습니다.\n"
            << "사용 예: UFFS_MOUNT_PATH=/mnt/uffs ./test_integration";

        /* 프로세스 고유 테스트 디렉토리 생성 */
        char dir[512];
        snprintf(dir, sizeof(dir), "%s/test_%d", g_mount, static_cast<int>(getpid()));
        test_dir_ = dir;

        if (mkdir(dir, 0755) != 0 && errno != EEXIST)
            FAIL() << "테스트 디렉토리 생성 실패: " << strerror(errno);
    }

    void TearDown() override {
        cleanup_dir(test_dir_.c_str());
    }

    /* test_dir_ 기준 절대 경로 반환 */
    std::string path(const char *name) const {
        return test_dir_ + "/" + name;
    }

    /* ─── /dev/urandom 으로 랜덤 데이터 생성 ─────────────── */
    static std::vector<uint8_t> random_data(size_t n) {
        std::vector<uint8_t> v(n);
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd < 0) return v;
        ssize_t r = read(fd, v.data(), n);
        close(fd);
        if (r != static_cast<ssize_t>(n)) v.clear();
        return v;
    }

    /* ─── 디렉토리 재귀 정리 ──────────────────────────────── */
    static void cleanup_dir(const char *dir) {
        DIR *d = opendir(dir);
        if (!d) return;
        struct dirent *ent;
        while ((ent = readdir(d)) != nullptr) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
            char buf[512];
            snprintf(buf, sizeof(buf), "%s/%s", dir, ent->d_name);
            struct stat st;
            if (lstat(buf, &st) == 0) {
                if (S_ISDIR(st.st_mode)) cleanup_dir(buf);
                else                     unlink(buf);
            }
        }
        closedir(d);
        rmdir(dir);
    }
};

/* ═══════════════════════════════════════════════════════════
 * TC-FILE-001 — 파일 열기 (FR-FILE-001)
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_FILE_001_Open) {
    std::string p = path("existing.txt");

    /* 사전 파일 생성 */
    int fd = open(p.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0) << "사전 파일 생성 실패: " << strerror(errno);
    close(fd);

    /* 기존 파일 열기 */
    fd = open(p.c_str(), O_RDONLY);
    EXPECT_GE(fd, 0) << "기존 파일 열기 실패: " << strerror(errno);
    if (fd >= 0) close(fd);
}

/* ═══════════════════════════════════════════════════════════
 * TC-FILE-002 — 파일 생성 (FR-FILE-002)
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_FILE_002_Create) {
    std::string p = path("test.txt");

    int fd = open(p.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0) << "파일 생성 실패: " << strerror(errno);
    close(fd);

    struct stat st;
    ASSERT_EQ(stat(p.c_str(), &st), 0) << "stat 실패: " << strerror(errno);
    EXPECT_EQ(st.st_size, 0) << "신규 파일 크기가 0이 아님";
}

/* ═══════════════════════════════════════════════════════════
 * TC-FILE-003 — 파일 읽기 (FR-FILE-003)
 * 랜덤 데이터 쓰기 → fsync → 읽기 → 원본과 비교
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_FILE_003_Read) {
    auto data = random_data(g_file_size);
    ASSERT_FALSE(data.empty()) << "/dev/urandom 읽기 실패";

    std::string p = path("read_test.bin");

    /* 쓰기 + fsync */
    int fd = open(p.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    ASSERT_EQ(static_cast<ssize_t>(g_file_size),
              write(fd, data.data(), g_file_size))
        << "write 실패: " << strerror(errno);
    ASSERT_EQ(fsync(fd), 0) << "fsync 실패: " << strerror(errno);
    close(fd);

    /* 읽기 */
    fd = open(p.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    std::vector<uint8_t> buf(g_file_size);
    ssize_t n = read(fd, buf.data(), g_file_size);
    close(fd);

    ASSERT_EQ(n, static_cast<ssize_t>(g_file_size))
        << "read 반환값 불일치: " << strerror(errno);
    EXPECT_EQ(memcmp(data.data(), buf.data(), g_file_size), 0)
        << "읽은 데이터가 원본과 다름 (sha256 기준 불일치)";
}

/* ═══════════════════════════════════════════════════════════
 * TC-FILE-004 — 파일 쓰기 (FR-FILE-004)
 * 쓰기 → lseek(0) → 읽기 → 비교
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_FILE_004_Write) {
    auto data = random_data(g_file_size);
    ASSERT_FALSE(data.empty());

    std::string p = path("write_test.bin");
    int fd = open(p.c_str(), O_CREAT | O_RDWR, 0644);
    ASSERT_GE(fd, 0);

    ssize_t written = write(fd, data.data(), g_file_size);
    EXPECT_EQ(written, static_cast<ssize_t>(g_file_size))
        << "write 반환값이 요청 크기와 다름";

    ASSERT_EQ(lseek(fd, 0, SEEK_SET), static_cast<off_t>(0));

    std::vector<uint8_t> buf(g_file_size);
    ssize_t n = read(fd, buf.data(), g_file_size);
    close(fd);

    ASSERT_EQ(n, static_cast<ssize_t>(g_file_size));
    EXPECT_EQ(memcmp(data.data(), buf.data(), g_file_size), 0)
        << "쓴 데이터와 읽은 데이터 불일치";
}

/* ═══════════════════════════════════════════════════════════
 * TC-FILE-006 — 파일 이름 변경 (FR-FILE-006)
 * rename → stat(dst) 성공, stat(src) ENOENT
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_FILE_006_Rename) {
    std::string src = path("src.txt");
    std::string dst = path("dst.txt");

    int fd = open(src.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    close(fd);

    ASSERT_EQ(rename(src.c_str(), dst.c_str()), 0)
        << "rename 실패: " << strerror(errno);

    struct stat st;
    EXPECT_EQ(stat(dst.c_str(), &st), 0) << "dst 파일이 존재하지 않음";

    int rc = stat(src.c_str(), &st);
    EXPECT_NE(rc, 0)     << "src 파일이 여전히 존재함";
    EXPECT_EQ(errno, ENOENT) << "예상 errno: ENOENT";
}

/* ═══════════════════════════════════════════════════════════
 * TC-FILE-007 — 파일 삭제 (FR-FILE-007)
 * unlink → stat ENOENT
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_FILE_007_Unlink) {
    std::string p = path("test.txt");

    int fd = open(p.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    close(fd);

    ASSERT_EQ(unlink(p.c_str()), 0) << "unlink 실패: " << strerror(errno);

    struct stat st;
    int rc = stat(p.c_str(), &st);
    EXPECT_NE(rc, 0)         << "삭제된 파일이 여전히 존재함";
    EXPECT_EQ(errno, ENOENT) << "예상 errno: ENOENT";
}

/* ═══════════════════════════════════════════════════════════
 * TC-DIR-002 — 디렉토리 읽기 (FR-DIR-002)
 * 파일 생성 → readdir → 파일 이름이 목록에 존재
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_DIR_002_Readdir) {
    std::string p = path("listed.txt");
    int fd = open(p.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    close(fd);

    DIR *dir = opendir(test_dir_.c_str());
    ASSERT_NE(dir, nullptr) << "opendir 실패: " << strerror(errno);

    bool found = false;
    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (strcmp(ent->d_name, "listed.txt") == 0) {
            found = true;
            break;
        }
    }
    closedir(dir);

    EXPECT_TRUE(found) << "생성한 파일이 readdir 목록에 없음";
}

/* ═══════════════════════════════════════════════════════════
 * TC-DIR-003 — 디렉토리 생성 (FR-DIR-003)
 * mkdir → stat S_ISDIR
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_DIR_003_Mkdir) {
    std::string d = path("newdir");

    ASSERT_EQ(mkdir(d.c_str(), 0755), 0)
        << "mkdir 실패: " << strerror(errno);

    struct stat st;
    ASSERT_EQ(stat(d.c_str(), &st), 0) << "stat 실패: " << strerror(errno);
    EXPECT_TRUE(S_ISDIR(st.st_mode))   << "S_ISDIR 가 false";
}

/* ═══════════════════════════════════════════════════════════
 * TC-DIR-004 — 디렉토리 삭제 (FR-DIR-004)
 * mkdir → rmdir → stat ENOENT
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_DIR_004_Rmdir) {
    std::string d = path("emptydir");

    ASSERT_EQ(mkdir(d.c_str(), 0755), 0);
    ASSERT_EQ(rmdir(d.c_str()), 0) << "rmdir 실패: " << strerror(errno);

    struct stat st;
    int rc = stat(d.c_str(), &st);
    EXPECT_NE(rc, 0)         << "삭제된 디렉토리가 여전히 존재함";
    EXPECT_EQ(errno, ENOENT) << "예상 errno: ENOENT";
}

/* ═══════════════════════════════════════════════════════════
 * TC-META-001 — 메타데이터 조회 (FR-META-001)
 * 파일 쓰기 + fsync → stat: st_size, S_ISREG, st_nlink 확인
 * ═══════════════════════════════════════════════════════════ */
TEST_F(UffsTest, TC_META_001_GetAttr) {
    std::string p = path("meta_test.bin");

    int fd = open(p.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);

    /* 고정 패턴 데이터로 크기 검증 */
    std::vector<uint8_t> data(g_file_size, 0xAB);
    ASSERT_EQ(static_cast<ssize_t>(g_file_size),
              write(fd, data.data(), g_file_size));
    ASSERT_EQ(fsync(fd), 0) << "fsync 실패: " << strerror(errno);
    close(fd);

    struct stat st;
    ASSERT_EQ(stat(p.c_str(), &st), 0) << "stat 실패: " << strerror(errno);

    EXPECT_EQ(st.st_size, static_cast<off_t>(g_file_size))
        << "st_size 불일치: expected=" << g_file_size
        << " actual=" << st.st_size;
    EXPECT_TRUE(S_ISREG(st.st_mode))   << "S_ISREG 가 false";
    EXPECT_GE(st.st_nlink, static_cast<nlink_t>(1)) << "st_nlink < 1";
}

/* ─── main ───────────────────────────────────────────────── */
int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    g_mount = getenv("UFFS_MOUNT_PATH");
    if (!g_mount) {
        fprintf(stderr,
            "\n[경고] UFFS_MOUNT_PATH 환경변수가 설정되지 않았습니다.\n"
            "       모든 테스트가 SKIPPED 처리됩니다.\n"
            "  예: UFFS_MOUNT_PATH=/mnt/uffs ./test_integration\n\n");
    }

    /* 파일 크기 재정의 */
    const char *sz = getenv("UFFS_TEST_FILE_SIZE");
    if (sz) g_file_size = static_cast<size_t>(atol(sz));

    return RUN_ALL_TESTS();
}
