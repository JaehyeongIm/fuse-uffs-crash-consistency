#!/usr/bin/env bash
set -euo pipefail

# ─── 색상 ─────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; CYAN='\033[0;36m'; NC='\033[0m'
info()  { echo -e "${CYAN}[INFO]${NC} $*"; }
pass()  { echo -e "${GREEN}[PASS]${NC} $*"; }
fail()  { echo -e "${RED}[FAIL]${NC} $*"; exit 1; }

# ─── 경로 설정 ────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="${SCRIPT_DIR}/uffs-crash-consistency"
BUILD_DIR="${SRC_DIR}/build"
UFFS_BIN="${BUILD_DIR}/uffs"
TESTS_DIR="${BUILD_DIR}/tests"
MOUNT_POINT="/mnt/uffs"
DEVICE="/tmp/flash.img"

# ─── 빌드 ─────────────────────────────────────────────────────
info "Step 1/8: 빌드"
mkdir -p "${BUILD_DIR}"
cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug > /dev/null
make -C "${BUILD_DIR}" -j"$(nproc)" > /dev/null
pass "빌드 완료"

# ─── 마운트 포인트 준비 ────────────────────────────────────────
sudo mkdir -p "${MOUNT_POINT}"

# ─── 통합 테스트 (GTest) ─────────────────────────────────────
info "Step 2/8: 통합 테스트 (TC-FILE/DIR/META 10종)"
rm -f "${DEVICE}"
"${UFFS_BIN}" "${DEVICE}" "${MOUNT_POINT}" &
UFFS_PID=$!
sleep 1
UFFS_MOUNT_PATH="${MOUNT_POINT}" "${TESTS_DIR}/test_integration"
fusermount3 -u "${MOUNT_POINT}" 2>/dev/null || true
wait "${UFFS_PID}" 2>/dev/null || true
pass "통합 테스트 완료"

# ─── TC-CRASH-005 (1MB) ───────────────────────────────────────
info "Step 3/8: TC-CRASH-005 (1MB)"
UFFS_BIN="${UFFS_BIN}" UFFS_DEVICE="${DEVICE}" UFFS_MOUNT="${MOUNT_POINT}" \
    bash "${TESTS_DIR}/tc_crash_005.sh"
pass "TC-CRASH-005 (1MB) 완료"

# ─── TC-CRASH-005 (4KB) ───────────────────────────────────────
info "Step 4/8: TC-CRASH-005 (4KB)"
UFFS_BIN="${UFFS_BIN}" UFFS_DEVICE="${DEVICE}" UFFS_MOUNT="${MOUNT_POINT}" \
    FILE_SIZE=4096 bash "${TESTS_DIR}/tc_crash_005.sh"
pass "TC-CRASH-005 (4KB) 완료"

# ─── TC-CRASH-FILE-ADV ────────────────────────────────────────
info "Step 5/8: TC-CRASH-FILE-ADV (5 시나리오)"
UFFS_BIN="${UFFS_BIN}" bash "${TESTS_DIR}/tc_crash_file_adv.sh"
pass "TC-CRASH-FILE-ADV 완료"

# ─── TC-CRASH-DIR-001 ─────────────────────────────────────────
info "Step 6/8: TC-CRASH-DIR-001 (디렉토리 fsync 크래시)"
UFFS_BIN="${UFFS_BIN}" \
    UFFS_DEVICE=/tmp/uffs_tcdir001.img \
    UFFS_MOUNT=/tmp/uffs_tcdir001_mnt \
    bash "${TESTS_DIR}/tc_crash_dir_001.sh"
pass "TC-CRASH-DIR-001 완료"

# ─── TC-CRASH-NEG-001 ─────────────────────────────────────────
info "Step 7/8: TC-CRASH-NEG-001 (비보장 경계 관측)"
UFFS_BIN="${UFFS_BIN}" \
    UFFS_DEVICE=/tmp/uffs_tcdir001.img \
    UFFS_MOUNT=/tmp/uffs_tcdir001_mnt \
    bash "${TESTS_DIR}/tc_crash_neg_001.sh"
pass "TC-CRASH-NEG-001 완료"

# ─── TC-REL-001 (1,000회) ─────────────────────────────────────
info "Step 8/8: TC-REL-001 (1,000회 power-cut)"
UFFS_BIN="${UFFS_BIN}" UFFS_DEVICE="${DEVICE}" UFFS_MOUNT="${MOUNT_POINT}" \
    ITERATIONS=1000 bash "${TESTS_DIR}/tc_rel_001.sh"
pass "TC-REL-001 완료"

# ─── 최종 결과 ────────────────────────────────────────────────
echo ""
echo "══════════════════════════════════════════"
pass "전체 테스트 완료 — 13 TC 전체 실행"
echo "══════════════════════════════════════════"
