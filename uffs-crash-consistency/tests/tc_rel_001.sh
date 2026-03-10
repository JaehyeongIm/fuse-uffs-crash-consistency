#!/usr/bin/env bash
# TC-REL-001: fsync 내구성 반복 테스트 (NFR-REL-001)
#
# 목적:
#   1,000회 이상의 power-cut 시나리오에서 데이터 손실 0건 검증.
#   TC-CRASH-005 절차 A를 반복 자동화한다.
#
# 환경변수:
#   UFFS_BIN       uffs 실행 파일 경로     (기본: ../build/uffs)
#   UFFS_DEVICE    플래시 이미지 파일      (기본: /tmp/uffs_tcrel001.img)
#   UFFS_MOUNT     마운트 포인트           (기본: /tmp/uffs_tcrel001_mnt)
#   ITERATIONS     반복 횟수               (기본: 1000)
#   FILE_SIZE      파일 크기(B)            (기본: 4096)
#   RESET_EVERY    N회마다 이미지 초기화   (기본: 0 = 초기화 안 함, GC 신뢰)
#
# 사용 예:
#   ITERATIONS=100 FILE_SIZE=4096 ./tc_rel_001.sh
#   ITERATIONS=1000 RESET_EVERY=50 ./tc_rel_001.sh

set -euo pipefail

UFFS_BIN="${UFFS_BIN:-../build/uffs}"
UFFS_DEVICE="${UFFS_DEVICE:-/tmp/uffs_tcrel001.img}"
UFFS_MOUNT="${UFFS_MOUNT:-/tmp/uffs_tcrel001_mnt}"
ITERATIONS="${ITERATIONS:-1000}"
FILE_SIZE="${FILE_SIZE:-4096}"
RESET_EVERY="${RESET_EVERY:-0}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
pass()  { echo -e "${GREEN}[PASS]${NC} $1"; }
fail()  { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }
info()  { echo -e "${YELLOW}[INFO]${NC} $1"; }

UFFS_PID=""
LOSS_COUNT=0
SIZE_MISMATCH_COUNT=0

cleanup() {
    [ -n "$UFFS_PID" ] && kill "$UFFS_PID" 2>/dev/null || true
    fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
        || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true
}
trap cleanup EXIT

# ─── 마운트/언마운트 헬퍼 ───────────────────────────────────
do_mount() {
    "$UFFS_BIN" "$UFFS_DEVICE" "$UFFS_MOUNT" -f &
    UFFS_PID=$!
    for i in $(seq 1 30); do
        mountpoint -q "$UFFS_MOUNT" 2>/dev/null && return 0
        sleep 0.1
    done
    fail "마운트 타임아웃"
}

do_unmount() {
    local pid="$UFFS_PID"
    UFFS_PID=""
    kill "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    sleep 0.1
    fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
        || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true
    for i in $(seq 1 20); do
        mountpoint -q "$UFFS_MOUNT" 2>/dev/null || return 0
        sleep 0.1
    done
}

crash_unmount() {
    local pid="$UFFS_PID"
    UFFS_PID=""
    kill -9 "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    sleep 0.1
    fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
        || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true
    sleep 0.1
}

reset_image() {
    rm -f "$UFFS_DEVICE"
    # 다음 마운트 시 flash_init이 자동 포맷
}

# ─── 사전 조건 확인 ─────────────────────────────────────────
[ -x "$UFFS_BIN" ] || fail "uffs 바이너리 없음: $UFFS_BIN"
which python3 >/dev/null 2>&1 || fail "python3 필요"

# ─── 초기화 ────────────────────────────────────────────────
info "TC-REL-001 시작: ITERATIONS=${ITERATIONS}, FILE_SIZE=${FILE_SIZE}B"
info "시작 시각: $(date '+%Y-%m-%d %H:%M:%S')"

mkdir -p "$UFFS_MOUNT"
fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
    || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true
reset_image

# ─── 최초 마운트 ────────────────────────────────────────────
do_mount

# ─── 반복 테스트 ────────────────────────────────────────────
for iter in $(seq 1 "$ITERATIONS"); do

    # RESET_EVERY 마다 이미지 초기화 (GC 대신 신선한 이미지 사용)
    if [ "$RESET_EVERY" -gt 0 ] && [ $((iter % RESET_EVERY)) -eq 1 ] && [ "$iter" -gt 1 ]; then
        do_unmount
        reset_image
        do_mount
    fi

    # 데이터 패턴 순환: 랜덤 / 전부 0 / 전부 0xFF
    PATTERN=$((iter % 3))

    # write + fsync 수행 (Python으로 정확한 fsync 보장)
    SHA_ORIG=$(python3 - <<PYEOF 2>/dev/null
import os, hashlib

size    = int("${FILE_SIZE}")
pattern = int("${PATTERN}")
fpath   = "${UFFS_MOUNT}/rel_test.bin"

if pattern == 0:
    data = os.urandom(size)
elif pattern == 1:
    data = bytes(size)
else:
    data = bytes([0xFF] * size)

fd = os.open(fpath, os.O_CREAT | os.O_WRONLY | os.O_TRUNC, 0o644)
os.write(fd, data)
os.fsync(fd)
os.close(fd)
print(hashlib.sha256(data).hexdigest())
PYEOF
    ) || { fail "이터레이션 ${iter}: write+fsync 실패"; }

    # kill -9 크래시 시뮬레이션
    crash_unmount

    # 재마운트
    do_mount

    # 데이터 검증
    RESULT=$(python3 - <<PYEOF 2>/dev/null
import os, hashlib

fpath    = "${UFFS_MOUNT}/rel_test.bin"
sha_orig = "${SHA_ORIG}"
size     = int("${FILE_SIZE}")

try:
    fd   = os.open(fpath, os.O_RDONLY)
    data = os.read(fd, size + 1)
    os.close(fd)
    sha  = hashlib.sha256(data).hexdigest()
    if sha == sha_orig and len(data) == size:
        print("OK")
    elif len(data) != size:
        print(f"SIZE_MISMATCH:{len(data)}")
    else:
        print(f"SHA_MISMATCH:{sha[:16]}")
except FileNotFoundError:
    print("LOST")
except Exception as e:
    print(f"ERROR:{e}")
PYEOF
    )

    if [ "$RESULT" != "OK" ]; then
        echo -e "${RED}[FAIL]${NC} 이터레이션 ${iter}/${ITERATIONS}: $RESULT (pattern=${PATTERN})"
        if echo "$RESULT" | grep -q "SIZE_MISMATCH"; then
            SIZE_MISMATCH_COUNT=$((SIZE_MISMATCH_COUNT + 1))
        else
            LOSS_COUNT=$((LOSS_COUNT + 1))
        fi
    fi

    # 진행률 출력 (100회마다)
    if [ $((iter % 100)) -eq 0 ]; then
        info "진행: ${iter}/${ITERATIONS} | 손실: ${LOSS_COUNT} | 크기불일치: ${SIZE_MISMATCH_COUNT}"
    fi

done

# ─── 최종 정리 ──────────────────────────────────────────────
do_unmount

# ─── 최종 판정 ──────────────────────────────────────────────
TOTAL_FAIL=$((LOSS_COUNT + SIZE_MISMATCH_COUNT))

echo
echo "──────────────────────────────────────────"
echo "TC-REL-001 결과 요약"
echo "──────────────────────────────────────────"
info "완료 시각:       $(date '+%Y-%m-%d %H:%M:%S')"
info "총 반복 횟수:    ${ITERATIONS}"
info "데이터 손실:     ${LOSS_COUNT}건"
info "크기 불일치:     ${SIZE_MISMATCH_COUNT}건"
info "총 실패 횟수:    ${TOTAL_FAIL}건"
echo "──────────────────────────────────────────"

if [ "$TOTAL_FAIL" -eq 0 ]; then
    pass "TC-REL-001: ${ITERATIONS}회 반복 power-cut 시나리오에서 데이터 손실 0건"
    pass "NFR-REL-001 인수 기준 충족: 데이터 손실률 0%"
else
    fail "TC-REL-001: ${TOTAL_FAIL}건 실패 (손실 ${LOSS_COUNT} + 크기불일치 ${SIZE_MISMATCH_COUNT})"
fi
