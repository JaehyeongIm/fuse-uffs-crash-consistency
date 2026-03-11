#!/usr/bin/env bash
# TC-CRASH-FILE-ADV: 파일 쓰기 Crash Consistency 심층 검증
#
# Related Requirements: FR-FILE-004, FR-FILE-005
#
# 5개 시나리오:
#   S1: Sub-page  (100B)  — RMW CoW 경로 (512B 미만 쓰기)
#   S2: Cross-block(20KB) — FILE 헤더→DATA 블록 경계 (15360B 초과)
#   S3: Overwrite (4KB)   — CoW 덮어쓰기 후 최신 버전 복구
#   S4: Append    (4+4KB) — 순차 추가 쓰기 2단계
#   S5: Multi-file(×3)   — 3개 파일 동시 crash 후 복구
#
# 환경변수:
#   UFFS_BIN    uffs 실행 파일 경로  (기본: ../build/uffs)
#
# 사용 예:
#   bash tc_crash_file_adv.sh
#   UFFS_BIN=../uffs bash tc_crash_file_adv.sh

set -uo pipefail

UFFS_BIN="${UFFS_BIN:-../build/uffs}"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
BLUE='\033[0;34m'; NC='\033[0m'

pass()  { echo -e "${GREEN}[PASS]${NC} $1"; }
fail()  { echo -e "${RED}[FAIL]${NC} $1"; }
info()  { echo -e "${YELLOW}[INFO]${NC} $1"; }
scene() { echo -e "\n${BLUE}══ $1 ══${NC}"; }

UFFS_PID=""
PASS_COUNT=0
FAIL_COUNT=0

# ─── 공통 마운트/언마운트 헬퍼 ──────────────────────────────────

cleanup_all() {
    [ -n "$UFFS_PID" ] && kill "$UFFS_PID" 2>/dev/null || true
    for mnt in /tmp/uffs_adv_*; do
        fusermount3 -uz "$mnt" 2>/dev/null \
            || fusermount -uz "$mnt" 2>/dev/null || true
    done
}
trap cleanup_all EXIT

do_mount() {
    local device="$1" mount="$2"
    rm -f "$device"
    mkdir -p "$mount"
    fusermount3 -uz "$mount" 2>/dev/null \
        || fusermount -uz "$mount" 2>/dev/null || true
    "$UFFS_BIN" "$device" "$mount" -f &
    UFFS_PID=$!
    for i in $(seq 1 30); do
        mountpoint -q "$mount" 2>/dev/null && return 0
        sleep 0.1
    done
    fail "마운트 타임아웃: $mount"; return 1
}

do_crash_remount() {
    local device="$1" mount="$2"
    info "kill -9 크래시 시뮬레이션"
    kill -9 "$UFFS_PID"
    wait "$UFFS_PID" 2>/dev/null || true
    UFFS_PID=""
    sleep 0.1
    fusermount3 -uz "$mount" 2>/dev/null \
        || fusermount -uz "$mount" 2>/dev/null || true
    sleep 0.2
    info "재마운트"
    "$UFFS_BIN" "$device" "$mount" -f &
    UFFS_PID=$!
    for i in $(seq 1 30); do
        mountpoint -q "$mount" 2>/dev/null && return 0
        sleep 0.1
    done
    fail "재마운트 타임아웃: $mount"; return 1
}

do_end() {
    local mount="$1"
    fusermount3 -u "$mount" 2>/dev/null \
        || fusermount -u "$mount" 2>/dev/null || true
    wait "$UFFS_PID" 2>/dev/null || true
    UFFS_PID=""
    sleep 0.2
}

check() {
    local name="$1" cond="$2"
    if [ "$cond" -eq 1 ]; then
        pass "$name"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        fail "$name"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

# ─── 사전 조건 확인 ──────────────────────────────────────────────
[ -x "$UFFS_BIN" ]                   || { fail "uffs 바이너리 없음: $UFFS_BIN"; exit 1; }
which python3 >/dev/null 2>&1        || { fail "python3 필요"; exit 1; }
python3 -c "import secrets" 2>/dev/null || { fail "python3 secrets 모듈 필요"; exit 1; }

echo ""
echo "  TC-CRASH-FILE-ADV: 파일 쓰기 Crash Consistency 심층 검증"
echo "  바이너리: $UFFS_BIN"
echo ""

# ════════════════════════════════════════════════════════════════
# S1: Sub-page write (100B) — RMW CoW 경로 검증
# ════════════════════════════════════════════════════════════════
scene "S1: Sub-page write (100B) — RMW CoW 경로"

DEV="/tmp/uffs_adv_s1.img"
MNT="/tmp/uffs_adv_s1"
do_mount "$DEV" "$MNT"

SHA_S1=$(python3 - <<PYEOF
import os, hashlib, secrets
data = secrets.token_bytes(100)
fd = os.open("${MNT}/sub.bin", os.O_CREAT | os.O_WRONLY, 0o644)
os.write(fd, data)
os.fsync(fd)
os.close(fd)
print(hashlib.sha256(data).hexdigest())
PYEOF
)
info "원본 SHA256 (100B): $SHA_S1"

do_crash_remount "$DEV" "$MNT"

RESULT_S1=$(python3 - <<PYEOF
import os, hashlib
try:
    fd = os.open("${MNT}/sub.bin", os.O_RDONLY)
    data = os.read(fd, 512)
    os.close(fd)
    print(f"{len(data)}:{hashlib.sha256(data).hexdigest()}")
except Exception as e:
    print(f"0:ERROR:{e}")
PYEOF
)
REC_SZ_S1=$(echo "$RESULT_S1" | cut -d: -f1)
REC_SHA_S1=$(echo "$RESULT_S1" | cut -d: -f2)
info "복구: size=${REC_SZ_S1}B  SHA256=${REC_SHA_S1}"

[ "$REC_SZ_S1" -eq 100 ] && [ "$REC_SHA_S1" = "$SHA_S1" ] && S1_OK=1 || S1_OK=0
do_end "$MNT"
check "S1 Sub-page (100B) fsync 후 복구 일치" $S1_OK

# ════════════════════════════════════════════════════════════════
# S2: Cross-block boundary (20KB) — FILE 헤더→DATA 블록 경계
# ════════════════════════════════════════════════════════════════
scene "S2: Cross-block boundary (20KB) — 15,360B 경계 초과"

DEV="/tmp/uffs_adv_s2.img"
MNT="/tmp/uffs_adv_s2"
do_mount "$DEV" "$MNT"

SHA_S2=$(python3 - <<PYEOF
import os, hashlib, secrets
data = secrets.token_bytes(20 * 1024)   # 20KB: FILE 헤더(15360B) 초과 → DATA 블록 할당
fd = os.open("${MNT}/cross.bin", os.O_CREAT | os.O_WRONLY, 0o644)
os.write(fd, data)
os.fsync(fd)
os.close(fd)
print(hashlib.sha256(data).hexdigest())
PYEOF
)
info "원본 SHA256 (20KB): $SHA_S2"

do_crash_remount "$DEV" "$MNT"

RESULT_S2=$(python3 - <<PYEOF
import os, hashlib
try:
    fd = os.open("${MNT}/cross.bin", os.O_RDONLY)
    data = os.read(fd, 30 * 1024)
    os.close(fd)
    print(f"{len(data)}:{hashlib.sha256(data).hexdigest()}")
except Exception as e:
    print(f"0:ERROR:{e}")
PYEOF
)
REC_SZ_S2=$(echo "$RESULT_S2" | cut -d: -f1)
REC_SHA_S2=$(echo "$RESULT_S2" | cut -d: -f2)
info "복구: size=${REC_SZ_S2}B  SHA256=${REC_SHA_S2}"

[ "$REC_SZ_S2" -eq $((20*1024)) ] && [ "$REC_SHA_S2" = "$SHA_S2" ] && S2_OK=1 || S2_OK=0
do_end "$MNT"
check "S2 Cross-block boundary (20KB) fsync 후 복구 일치" $S2_OK

# ════════════════════════════════════════════════════════════════
# S3: Overwrite — CoW 최신 버전 선택 검증
#   Phase 1: pattern 0xAA 4KB write + fsync
#   Phase 2: pattern 0xBB 4KB overwrite + fsync → kill-9
#   기대: 복구 후 0xBB (최신 버전)
# ════════════════════════════════════════════════════════════════
scene "S3: Overwrite (0xAA→0xBB) — CoW 최신 버전 선택"

DEV="/tmp/uffs_adv_s3.img"
MNT="/tmp/uffs_adv_s3"
do_mount "$DEV" "$MNT"

SHAS_S3=$(python3 - <<PYEOF
import os, hashlib
fpath = "${MNT}/ow.bin"

# Phase 1: pattern A (0xAA)
data_a = bytes([0xAA] * 4096)
fd = os.open(fpath, os.O_CREAT | os.O_WRONLY, 0o644)
os.write(fd, data_a)
os.fsync(fd)
os.close(fd)

# Phase 2: pattern B (0xBB) — 동일 오프셋 덮어쓰기
data_b = bytes([0xBB] * 4096)
fd = os.open(fpath, os.O_WRONLY)
os.lseek(fd, 0, os.SEEK_SET)
os.write(fd, data_b)
os.fsync(fd)
os.close(fd)

print(f"{hashlib.sha256(data_a).hexdigest()}:{hashlib.sha256(data_b).hexdigest()}")
PYEOF
)
SHA_A=$(echo "$SHAS_S3" | cut -d: -f1)
SHA_B=$(echo "$SHAS_S3" | cut -d: -f2)
info "Pattern A (0xAA) SHA256: $SHA_A"
info "Pattern B (0xBB) SHA256: $SHA_B  ← 복구 기대값"

do_crash_remount "$DEV" "$MNT"

RESULT_S3=$(python3 - <<PYEOF
import os, hashlib
try:
    fd = os.open("${MNT}/ow.bin", os.O_RDONLY)
    data = os.read(fd, 8192)
    os.close(fd)
    print(f"{len(data)}:{hashlib.sha256(data).hexdigest()}")
except Exception as e:
    print(f"0:ERROR:{e}")
PYEOF
)
REC_SZ_S3=$(echo "$RESULT_S3" | cut -d: -f1)
REC_SHA_S3=$(echo "$RESULT_S3" | cut -d: -f2)
info "복구: size=${REC_SZ_S3}B  SHA256=${REC_SHA_S3}"

[ "$REC_SZ_S3" -eq 4096 ] && [ "$REC_SHA_S3" = "$SHA_B" ] && S3_OK=1 || S3_OK=0
do_end "$MNT"
check "S3 Overwrite: fsync된 최신 버전(0xBB) 복구 확인" $S3_OK

# ════════════════════════════════════════════════════════════════
# S4: Append — 순차 추가 쓰기 2단계
#   Step 1: 4KB write + fsync
#   Step 2: 4KB append + fsync → kill-9
#   기대: 복구 후 8KB 전체 일치
# ════════════════════════════════════════════════════════════════
scene "S4: Append (4KB + 4KB) — 순차 추가 쓰기"

DEV="/tmp/uffs_adv_s4.img"
MNT="/tmp/uffs_adv_s4"
do_mount "$DEV" "$MNT"

SHA_S4=$(python3 - <<PYEOF
import os, hashlib, secrets
fpath = "${MNT}/append.bin"

chunk1 = secrets.token_bytes(4096)
chunk2 = secrets.token_bytes(4096)

# Step 1: 첫 번째 4KB
fd = os.open(fpath, os.O_CREAT | os.O_WRONLY, 0o644)
os.write(fd, chunk1)
os.fsync(fd)
os.close(fd)

# Step 2: 두 번째 4KB append
fd = os.open(fpath, os.O_WRONLY | os.O_APPEND)
os.write(fd, chunk2)
os.fsync(fd)
os.close(fd)

print(hashlib.sha256(chunk1 + chunk2).hexdigest())
PYEOF
)
info "원본 SHA256 (8KB = 4KB+4KB): $SHA_S4"

do_crash_remount "$DEV" "$MNT"

RESULT_S4=$(python3 - <<PYEOF
import os, hashlib
try:
    fd = os.open("${MNT}/append.bin", os.O_RDONLY)
    data = os.read(fd, 16384)
    os.close(fd)
    print(f"{len(data)}:{hashlib.sha256(data).hexdigest()}")
except Exception as e:
    print(f"0:ERROR:{e}")
PYEOF
)
REC_SZ_S4=$(echo "$RESULT_S4" | cut -d: -f1)
REC_SHA_S4=$(echo "$RESULT_S4" | cut -d: -f2)
info "복구: size=${REC_SZ_S4}B  SHA256=${REC_SHA_S4}"

[ "$REC_SZ_S4" -eq 8192 ] && [ "$REC_SHA_S4" = "$SHA_S4" ] && S4_OK=1 || S4_OK=0
do_end "$MNT"
check "S4 Append (8KB): 두 단계 fsync 후 전체 복구 일치" $S4_OK

# ════════════════════════════════════════════════════════════════
# S5: Multi-file — 3개 파일 동시 crash 후 복구
#   각 파일을 write + fsync 후 kill-9
#   기대: 3개 파일 모두 각각의 SHA256과 일치
# ════════════════════════════════════════════════════════════════
scene "S5: Multi-file (3개) — 동시 crash 후 전체 복구"

DEV="/tmp/uffs_adv_s5.img"
MNT="/tmp/uffs_adv_s5"
do_mount "$DEV" "$MNT"

SHAS_S5=$(python3 - <<PYEOF
import os, hashlib, secrets
mount = "${MNT}"
results = []
for i in range(3):
    data = secrets.token_bytes(4096)
    fd = os.open(f"{mount}/file{i}.bin", os.O_CREAT | os.O_WRONLY, 0o644)
    os.write(fd, data)
    os.fsync(fd)
    os.close(fd)
    results.append(hashlib.sha256(data).hexdigest())
print("|".join(results))
PYEOF
)
SHA_F0=$(echo "$SHAS_S5" | cut -d'|' -f1)
SHA_F1=$(echo "$SHAS_S5" | cut -d'|' -f2)
SHA_F2=$(echo "$SHAS_S5" | cut -d'|' -f3)
info "file0 SHA256: $SHA_F0"
info "file1 SHA256: $SHA_F1"
info "file2 SHA256: $SHA_F2"

do_crash_remount "$DEV" "$MNT"

RESULTS_S5=$(python3 - <<PYEOF
import os, hashlib
mount = "${MNT}"
out = []
for i in range(3):
    try:
        fd = os.open(f"{mount}/file{i}.bin", os.O_RDONLY)
        data = os.read(fd, 8192)
        os.close(fd)
        out.append(f"{len(data)}:{hashlib.sha256(data).hexdigest()}")
    except Exception as e:
        out.append(f"0:ERROR")
print("|".join(out))
PYEOF
)

S5_OK=1
EXPECTED_SHAS=("$SHA_F0" "$SHA_F1" "$SHA_F2")
for i in 0 1 2; do
    ENTRY=$(echo "$RESULTS_S5" | cut -d'|' -f$((i+1)))
    SZ=$(echo "$ENTRY" | cut -d: -f1)
    SHA=$(echo "$ENTRY" | cut -d: -f2)
    EXPECTED="${EXPECTED_SHAS[$i]}"
    if [ "$SZ" -eq 4096 ] && [ "$SHA" = "$EXPECTED" ]; then
        info "  file${i}: OK (4096B)"
    else
        info "  file${i}: FAIL (size=${SZ}, sha_match=$([ "$SHA" = "$EXPECTED" ] && echo true || echo false))"
        S5_OK=0
    fi
done

do_end "$MNT"
check "S5 Multi-file (3개): 모든 파일 복구 일치" $S5_OK

# ════════════════════════════════════════════════════════════════
# 최종 요약
# ════════════════════════════════════════════════════════════════
TOTAL=$((PASS_COUNT + FAIL_COUNT))
echo ""
echo "══════════════════════════════════════════════════"
echo "  TC-CRASH-FILE-ADV 결과 요약"
echo "══════════════════════════════════════════════════"
printf "  S1 Sub-page  (100B)  : %s\n" "$([ "${S1_OK}" -eq 1 ] && echo PASS || echo FAIL)"
printf "  S2 Cross-block(20KB) : %s\n" "$([ "${S2_OK}" -eq 1 ] && echo PASS || echo FAIL)"
printf "  S3 Overwrite (CoW)   : %s\n" "$([ "${S3_OK}" -eq 1 ] && echo PASS || echo FAIL)"
printf "  S4 Append    (8KB)   : %s\n" "$([ "${S4_OK}" -eq 1 ] && echo PASS || echo FAIL)"
printf "  S5 Multi-file(×3)    : %s\n" "$([ "${S5_OK}" -eq 1 ] && echo PASS || echo FAIL)"
echo "  ──────────────────────────────────────────────"
echo "  통과: ${PASS_COUNT} / ${TOTAL}"
echo "══════════════════════════════════════════════════"

if [ "$PASS_COUNT" -eq "$TOTAL" ]; then
    pass "TC-CRASH-FILE-ADV: ${TOTAL}개 시나리오 전체 통과 — Crash Consistency 심층 검증 완료"
    exit 0
else
    fail "TC-CRASH-FILE-ADV: ${FAIL_COUNT}개 시나리오 실패"
    exit 1
fi
