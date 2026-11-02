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
  "${QEMU_RISCV64}" -cpu "max,v=false,zvbce=false" \
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
    -cpu "max,v=true,vlen=${vlen},zvbce=${extension}" \
    "${BUILD_DIR}/${executable}"
}

expect_feature_gate_failure() {
  local executable=$1
  local label=$2
  local error_log
  local shell_log
  local status=0
  error_log=$(mktemp)
  shell_log=$(mktemp)

  bash -c '"$1" -cpu "max,v=true,vlen=128,zvbce=false" "$2" \
    >/dev/null 2>"$3"' \
    _ "${QEMU_RISCV64}" "${BUILD_DIR}/${executable}" "${error_log}" \
    2>"${shell_log}" || status=$?

  if [[ ${status} -ne 132 ]] ||
     ! grep -Eiq "Illegal instruction|signal 4" "${error_log}" "${shell_log}"; then
    echo "error: ${label} did not fail with the expected SIGILL " \
         "(status=${status})" >&2
    cat "${error_log}" >&2
    cat "${shell_log}" >&2
    rm -f "${error_log}"
    rm -f "${shell_log}"
    exit 1
  fi

  rm -f "${error_log}" "${shell_log}"
  echo "${label} feature-gate check passed (SIGILL, status=${status})"
}

run_scalar

echo
echo "=== P3104R0 portable C++ implementation ==="
"${QEMU_RISCV64}" -cpu "max,v=false,zvbce=false" \
  "${BUILD_DIR}/run-p3104"

for vlen in ${VLENS}; do
  run_vector "${vlen}" false run-rvv "RVV without Zvbce"
  run_vector "${vlen}" true run-zvbce "RVV + Zvbce"
  run_vector "${vlen}" true run-p3104-zvbce \
    "P3104R0 C++ API + Zvbce"
done

echo
echo "Checking that the Zvbce binary fails when Zvbce is disabled"
expect_feature_gate_failure run-zvbce run-zvbce

echo
echo "Checking that the P3104R0 Zvbce binary fails when Zvbce is disabled"
expect_feature_gate_failure run-p3104-zvbce run-p3104-zvbce
