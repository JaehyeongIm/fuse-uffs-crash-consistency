#!/usr/bin/env bash
# TC-CRASH-005: fsync(fd) 후 프로세스 크래시 내구성
#
# Related Requirements: FR-FILE-005, NFR-REL-001
#
# 환경변수:
#   UFFS_BIN    uffs 실행 파일 경로  (기본: ../build/uffs)
#   UFFS_DEVICE 플래시 이미지 파일   (기본: /tmp/uffs_tc005.img)
#   UFFS_MOUNT  마운트 포인트        (기본: /tmp/uffs_tc005_mnt)
#   FILE_SIZE   테스트 파일 크기(B)  (기본: 1048576 = 1MB)
#
# 사용 예:
#   ./tc_crash_005.sh
#   UFFS_BIN=./build/uffs FILE_SIZE=65536 ./tc_crash_005.sh

set -euo pipefail

UFFS_BIN="${UFFS_BIN:-../build/uffs}"
UFFS_DEVICE="${UFFS_DEVICE:-/tmp/uffs_tc005.img}"
UFFS_MOUNT="${UFFS_MOUNT:-/tmp/uffs_tc005_mnt}"
FILE_SIZE="${FILE_SIZE:-1048576}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

pass()  { echo -e "${GREEN}[PASS]${NC} $1"; }
fail()  { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }
info()  { echo -e "${YELLOW}[INFO]${NC} $1"; }

TMP_ORIG="/tmp/uffs_tc005_orig.bin"
UFFS_PID=""

# ─── 정리 함수 ──────────────────────────────────────────────
cleanup() {
    [ -n "$UFFS_PID" ] && kill "$UFFS_PID" 2>/dev/null || true
    fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
        || fusermount -uz "$UFFS_MOUNT" 2>/dev/null \
        || true
    rm -f "$TMP_ORIG"
}
trap cleanup EXIT

# ─── 마운트 헬퍼 ────────────────────────────────────────────
do_mount() {
    "$UFFS_BIN" "$UFFS_DEVICE" "$UFFS_MOUNT" -f &
    UFFS_PID=$!
    # 마운트 준비 대기 (최대 3초)
    for i in $(seq 1 30); do
        mountpoint -q "$UFFS_MOUNT" 2>/dev/null && return 0
        sleep 0.1
    done
    fail "마운트 타임아웃 (3초)"
}

do_unmount() {
    local pid="$UFFS_PID"
    UFFS_PID=""
    kill "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    sleep 0.1
    fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
        || fusermount -uz "$UFFS_MOUNT" 2>/dev/null \
        || true
    # 마운트 해제 확인
    for i in $(seq 1 20); do
        mountpoint -q "$UFFS_MOUNT" 2>/dev/null || return 0
        sleep 0.1
    done
}

# ─── 사전 조건 확인 ─────────────────────────────────────────
[ -x "$UFFS_BIN" ] || fail "uffs 바이너리 없음: $UFFS_BIN"
which python3 >/dev/null 2>&1 || fail "python3 필요"

# ─── 환경 초기화 ────────────────────────────────────────────
info "TC-CRASH-005 시작: FILE_SIZE=${FILE_SIZE}B"

rm -f "$UFFS_DEVICE"
mkdir -p "$UFFS_MOUNT"
fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
    || fusermount -uz "$UFFS_MOUNT" 2>/dev/null \
    || true

# ─── Step 1: 마운트 ─────────────────────────────────────────
info "Step 1: 마운트"
do_mount

# ─── Step 2: 랜덤 데이터 생성 및 fsync ─────────────────────
info "Step 2: ${FILE_SIZE}B 데이터 write + fsync"
SHA_ORIG=$(python3 - <<PYEOF
import os, hashlib, sys

size   = int("${FILE_SIZE}")
mount  = "${UFFS_MOUNT}"
fpath  = mount + "/tc005_test.bin"
tmp    = "${TMP_ORIG}"

data = os.urandom(size)

fd = os.open(fpath, os.O_CREAT | os.O_WRONLY, 0o644)
os.write(fd, data)
os.fsync(fd)
os.close(fd)

# 원본 sha256 출력 (bash에서 캡처)
print(hashlib.sha256(data).hexdigest())
PYEOF
)
info "원본 SHA256: $SHA_ORIG"

# ─── Step 3: 프로세스 크래시 시뮬레이션 (kill -9) ──────────
info "Step 3: kill -9 (크래시 시뮬레이션)"
kill -9 "$UFFS_PID"
wait "$UFFS_PID" 2>/dev/null || true
UFFS_PID=""
sleep 0.1

# ─── Step 4: 마운트 해제 ────────────────────────────────────
info "Step 4: 마운트 해제"
fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
    || fusermount -uz "$UFFS_MOUNT" 2>/dev/null \
    || true
sleep 0.2

# ─── Step 5: 재마운트 ───────────────────────────────────────
info "Step 5: 재마운트"
do_mount

# ─── Step 6: 데이터 검증 ────────────────────────────────────
info "Step 6: 데이터 검증"
SHA_AFTER=$(sha256sum "${UFFS_MOUNT}/tc005_test.bin" | awk '{print $1}')
SIZE_AFTER=$(stat -c%s "${UFFS_MOUNT}/tc005_test.bin" 2>/dev/null || echo "0")

info "복구 SHA256: $SHA_AFTER"
info "복구 크기:   ${SIZE_AFTER}B"

do_unmount

# ─── 판정 ──────────────────────────────────────────────────
[ "$SHA_ORIG" = "$SHA_AFTER" ] \
    || fail "데이터 손실: SHA256 불일치\n  원본: $SHA_ORIG\n  복구: $SHA_AFTER"

[ "$SIZE_AFTER" = "$FILE_SIZE" ] \
    || fail "파일 크기 불일치: expected=${FILE_SIZE} actual=${SIZE_AFTER}"

pass "TC-CRASH-005: fsync(fd) 후 크래시에서 데이터 완전 보존"
