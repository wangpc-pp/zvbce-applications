#!/usr/bin/env bash

set -euo pipefail
ulimit -c 0

if [[ $# -gt 1 ]]; then
  echo "usage: $0 [build-directory]" >&2
  exit 2
fi

BUILD_DIR=${1:-${BUILD_DIR:-build}}
: "${QEMU_RISCV64:?Set QEMU_RISCV64 to the qemu-riscv64 executable}"

VLENS=${VLENS:-"128 256 512"}

if [[ ! -x ${QEMU_RISCV64} ]]; then
  echo "error: QEMU_RISCV64 is not executable: ${QEMU_RISCV64}" >&2
  exit 1
fi

run_scalar() {
  echo "=== Scalar C reference ==="
  "${QEMU_RISCV64}" -cpu "max,v=false,zvbext=false" \
    "${BUILD_DIR}/run-scalar"
}

run_vector() {
  local vlen=$1
  local extension=$2
  local executable=$3
  local label=$4

  echo
  echo "=== VLEN=${vlen}: ${label} ==="
  "${QEMU_RISCV64}" \
    -cpu "max,v=true,vlen=${vlen},zvbext=${extension}" \
    "${BUILD_DIR}/${executable}"
}

run_scalar

for vlen in ${VLENS}; do
  run_vector "${vlen}" false run-rvv "RVV without Zvbext"
  run_vector "${vlen}" true run-zvbext "RVV + Zvbext"
done

echo
echo "Checking that the Zvbext binary fails when Zvbext is disabled"
negative_status=0
bash -c '"$1" -cpu "max,v=true,vlen=128,zvbext=false" "$2" >/dev/null 2>&1' \
  _ "${QEMU_RISCV64}" "${BUILD_DIR}/run-zvbext" 2>/dev/null ||
  negative_status=$?

if [[ ${negative_status} -eq 0 ]]; then
  echo "error: run-zvbext unexpectedly succeeded with zvbext=false" >&2
  exit 1
fi

echo "Negative feature-gate check passed (status=${negative_status})"
