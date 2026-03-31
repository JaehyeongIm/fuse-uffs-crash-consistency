#!/usr/bin/env bash
# TC-CRASH-NEG-001: fsync 이전 크래시 — 비보장 경계 음성 테스트
#
# Related Requirements: FR-FILE-005-3
#
# 목적:
#   fsync 호출 없이 크래시 발생 시 결과를 보장하지 않음을 확인하는 음성 테스트.
#   합격/불합격 판정 대상이 아니며, 관측 결과를 기록하여 비보장 범위를 문서화한다.
#
# 출력:
#   "preserved" 또는 "lost" — 판정 없음, 기록용

set -euo pipefail

UFFS_BIN="${UFFS_BIN:-../build/uffs}"
UFFS_DEVICE="${UFFS_DEVICE:-/tmp/uffs_tcneg001.img}"
UFFS_MOUNT="${UFFS_MOUNT:-/tmp/uffs_tcneg001_mnt}"
FILE_SIZE="${FILE_SIZE:-4096}"

YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
info() { echo -e "${YELLOW}[INFO]${NC} $1"; }
obs()  { echo -e "${CYAN}[OBS] ${NC} $1"; }

UFFS_PID=""

cleanup() {
    [ -n "$UFFS_PID" ] && kill "$UFFS_PID" 2>/dev/null || true
    fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
        || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true
}
trap cleanup EXIT

do_mount() {
    "$UFFS_BIN" "$UFFS_DEVICE" "$UFFS_MOUNT" -f &
    UFFS_PID=$!
    for i in $(seq 1 30); do
        mountpoint -q "$UFFS_MOUNT" 2>/dev/null && return 0
        sleep 0.1
    done
    echo "[ERR] 마운트 타임아웃"; exit 1
}

[ -x "$UFFS_BIN" ] || { echo "[ERR] uffs 바이너리 없음: $UFFS_BIN"; exit 1; }
which python3 >/dev/null 2>&1 || { echo "[ERR] python3 필요"; exit 1; }

info "TC-CRASH-NEG-001 시작 (음성 테스트 — 결과는 판정 대상 아님)"

rm -f "$UFFS_DEVICE"
mkdir -p "$UFFS_MOUNT"
fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
    || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true

# ─── Step 1: 마운트 ─────────────────────────────────────────
info "Step 1: 마운트"
do_mount

# ─── Step 2: write (fsync 없음) ─────────────────────────────
info "Step 2: write 수행 — fsync 호출 없음"
SHA_ORIG=$(python3 - <<PYEOF
import os, hashlib

data  = os.urandom(int("${FILE_SIZE}"))
fpath = "${UFFS_MOUNT}/neg_test.bin"

fd = os.open(fpath, os.O_CREAT | os.O_WRONLY, 0o644)
os.write(fd, data)
# os.fsync(fd) ← 의도적으로 생략
os.close(fd)

print(hashlib.sha256(data).hexdigest())
PYEOF
)
info "write 완료 (fsync 미호출). 원본 SHA256: $SHA_ORIG"

# ─── Step 3: kill -9 ────────────────────────────────────────
info "Step 3: kill -9 크래시 시뮬레이션"
kill -9 "$UFFS_PID"
wait "$UFFS_PID" 2>/dev/null || true
UFFS_PID=""
sleep 0.1

# ─── Step 4: 재마운트 ───────────────────────────────────────
info "Step 4: 재마운트"
fusermount3 -uz "$UFFS_MOUNT" 2>/dev/null \
    || fusermount -uz "$UFFS_MOUNT" 2>/dev/null || true
sleep 0.2
do_mount

# ─── Step 5: 결과 관측 (판정 없음) ─────────────────────────
OBSERVED=$(python3 - <<PYEOF
import os, hashlib

fpath     = "${UFFS_MOUNT}/neg_test.bin"
sha_orig  = "${SHA_ORIG}"
file_size = int("${FILE_SIZE}")

try:
    fd = os.open(fpath, os.O_RDONLY)
    data = os.read(fd, file_size + 1)
    os.close(fd)
    sha_after = hashlib.sha256(data).hexdigest()
    if sha_after == sha_orig and len(data) == file_size:
        print(f"preserved (size={len(data)}, sha256_match=True)")
    else:
        print(f"partial  (size={len(data)}, sha256_match={sha_after==sha_orig})")
except FileNotFoundError:
    print("lost (파일 없음)")
except Exception as e:
    print(f"error: {e}")
PYEOF
)

obs "관측 결과: $OBSERVED"
obs "이 케이스는 합격/불합격 판정 대상이 아닙니다 (FR-FILE-005-3 비보장 경계)."
obs "결과를 test-result.md 에 기록하십시오."

echo
echo "TC-CRASH-NEG-001 완료 (음성 테스트, 판정 없음)"
