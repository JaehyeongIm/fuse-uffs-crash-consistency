#!/usr/bin/env bash
set -euo pipefail

CYAN='\033[0;36m'; GREEN='\033[0;32m'; NC='\033[0m'
info() { echo -e "${CYAN}[INFO]${NC} $*"; }
pass() { echo -e "${GREEN}[PASS]${NC} $*"; }

info "패키지 목록 업데이트"
sudo apt-get update

info "의존성 설치"
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libfuse3-dev \
    fuse3 \
    libgtest-dev

info "마운트 포인트 생성"
sudo mkdir -p /mnt/uffs
sudo chown "$USER" /mnt/uffs

pass "의존성 설치 완료"
