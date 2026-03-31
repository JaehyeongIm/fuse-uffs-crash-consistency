#!/usr/bin/env bash
set -euo pipefail

CYAN='\033[0;36m'; GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'
info() { echo -e "${CYAN}[INFO]${NC} $*"; }
pass() { echo -e "${GREEN}[PASS]${NC} $*"; }
fail() { echo -e "${RED}[FAIL]${NC} $*"; exit 1; }

ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC="$ROOT/uffs-crash-consistency"
BUILD="$SRC/build"
BIN="$BUILD/uffs"
TESTS="$BUILD/tests"
DEVICE=/tmp/flash.img
MOUNT=/mnt/uffs

# ── 빌드 ────────────────────────────────────────────────────
info "빌드 시작"
mkdir -p "$BUILD"
cmake -S "$SRC" -B "$BUILD" -DCMAKE_BUILD_TYPE=Debug -Wno-dev > /dev/null
make -C "$BUILD" -j"$(nproc)" > /dev/null
pass "빌드 완료"

# ── Integration Test ─────────────────────────────────────────
info "Integration Test 시작"
rm -f "$DEVICE"
"$BIN" "$DEVICE" "$MOUNT" &
UFFS_PID=$!
sleep 1
UFFS_MOUNT_PATH="$MOUNT" "$TESTS/test_integration"
fusermount3 -u "$MOUNT" 2>/dev/null || true
wait "$UFFS_PID" 2>/dev/null || true
pass "Integration Test 완료"

# ── TC-CRASH-005 ─────────────────────────────────────────────
info "TC-CRASH-005 (1MB) 시작"
UFFS_BIN="$BIN" UFFS_DEVICE="$DEVICE" UFFS_MOUNT="$MOUNT" \
    bash "$TESTS/tc_crash_005.sh"

info "TC-CRASH-005 (4KB) 시작"
UFFS_BIN="$BIN" UFFS_DEVICE="$DEVICE" UFFS_MOUNT="$MOUNT" FILE_SIZE=4096 \
    bash "$TESTS/tc_crash_005.sh"

# ── TC-CRASH-FILE-ADV ────────────────────────────────────────
info "TC-CRASH-FILE-ADV 시작"
UFFS_BIN="$BIN" \
    bash "$TESTS/tc_crash_file_adv.sh"

# ── TC-CRASH-DIR-001 ─────────────────────────────────────────
info "TC-CRASH-DIR-001 시작"
UFFS_BIN="$BIN" UFFS_DEVICE=/tmp/uffs_tcdir001.img UFFS_MOUNT=/tmp/uffs_tcdir001_mnt \
    bash "$TESTS/tc_crash_dir_001.sh"

# ── TC-CRASH-NEG-001 ─────────────────────────────────────────
info "TC-CRASH-NEG-001 시작 (관측 전용)"
UFFS_BIN="$BIN" UFFS_DEVICE=/tmp/uffs_tcdir001.img UFFS_MOUNT=/tmp/uffs_tcdir001_mnt \
    bash "$TESTS/tc_crash_neg_001.sh"

# ── TC-REL-001 ───────────────────────────────────────────────
info "TC-REL-001 (1,000회) 시작"
UFFS_BIN="$BIN" UFFS_DEVICE="$DEVICE" UFFS_MOUNT="$MOUNT" ITERATIONS=1000 \
    bash "$TESTS/tc_rel_001.sh"

pass "전체 테스트 완료"
