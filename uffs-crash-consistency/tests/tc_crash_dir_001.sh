#!/usr/bin/env bash
# TC-CRASH-DIR-001: fsync(dirfd) 후 프로세스 크래시 — 디렉토리 엔트리 내구성
#
# Related Requirements: FR-DIR-001
#
# 절차:
#   1. 파일 생성 (new_file.txt)
#   2. 부모 디렉토리 fd 획득 후 fsync(dirfd)
#   3. kill -9 크래시 시뮬레이션
#   4. 재마운트
#   5. new_file.txt 가 readdir 목록에 동일하게 관측됨을 확인

set -euo pipefail

UFFS_BIN="${UFFS_BIN:-../build/uffs}"
UFFS_DEVICE="${UFFS_DEVICE:-/tmp/uffs_tcdir001.img}"
UFFS_MOUNT="${UFFS_MOUNT:-/tmp/uffs_tcdir001_mnt}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
pass()  { echo -e "${GREEN}[PASS]${NC} $1"; }
fail()  { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }
info()  { echo -e "${YELLOW}[INFO]${NC} $1"; }

UFFS_PID=""

cleanup() {
    [ -n "$UFFS_PID" ] && kill "$UFFS_PID" 2>/dev/null || true
    fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
        || fusermount -uz "$UFFS_MOUNT" 2>/dev/null \
        || true
}
trap cleanup EXIT

do_mount() {
    "$UFFS_BIN" "$UFFS_DEVICE" "$UFFS_MOUNT" -f &
    UFFS_PID=$!
    for i in $(seq 1 30); do
        mountpoint -q "$UFFS_MOUNT" 2>/dev/null && return 0
        sleep 0.1
    done
    fail "마운트 타임아웃"
}

# ─── 사전 조건 확인 ─────────────────────────────────────────
[ -x "$UFFS_BIN" ] || fail "uffs 바이너리 없음: $UFFS_BIN"
which python3 >/dev/null 2>&1 || fail "python3 필요"

info "TC-CRASH-DIR-001 시작"

rm -f "$UFFS_DEVICE"
mkdir -p "$UFFS_MOUNT"
fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
    || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true

# ─── Step 1: 마운트 ─────────────────────────────────────────
info "Step 1: 마운트"
do_mount

# ─── Step 2: 파일 생성 + fsync(dirfd) ───────────────────────
info "Step 2: new_file.txt 생성 + fsync(dirfd)"
python3 - <<PYEOF
import os

mount  = "${UFFS_MOUNT}"
fpath  = mount + "/new_file.txt"

# 파일 생성
fd = os.open(fpath, os.O_CREAT | os.O_WRONLY, 0o644)
os.write(fd, b"crash-dir-test")
os.fsync(fd)
os.close(fd)

# 부모 디렉토리 fsync (FR-DIR-001)
dirfd = os.open(mount, os.O_RDONLY)
os.fsync(dirfd)
os.close(dirfd)
print("fsync(dirfd) 완료")
PYEOF

# ─── Step 3: kill -9 ────────────────────────────────────────
info "Step 3: kill -9 크래시 시뮬레이션"
kill -9 "$UFFS_PID"
wait "$UFFS_PID" 2>/dev/null || true
UFFS_PID=""
sleep 0.1

# ─── Step 4: 마운트 해제 + 재마운트 ────────────────────────
info "Step 4: 재마운트"
fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
    || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true
sleep 0.2
do_mount

# ─── Step 5: 디렉토리 엔트리 검증 ──────────────────────────
info "Step 5: new_file.txt readdir/open 확인"
FOUND=$(python3 - <<PYEOF
import os

mount = "${UFFS_MOUNT}"

# readdir 확인
entries = os.listdir(mount)
print("디렉토리 엔트리:", entries)

if "new_file.txt" in entries:
    # open 확인
    fd = os.open(mount + "/new_file.txt", os.O_RDONLY)
    data = os.read(fd, 64)
    os.close(fd)
    print("FOUND:" + data.decode())
else:
    print("MISSING")
PYEOF
)

info "결과: $FOUND"

# 판정
echo "$FOUND" | grep -q "FOUND" || fail "재마운트 후 new_file.txt 가 관측되지 않음"

do_unmount 2>/dev/null || true

pass "TC-CRASH-DIR-001: fsync(dirfd) 후 크래시에서 디렉토리 엔트리 보존 확인"
