#!/usr/bin/env bash

set -euo pipefail

if [[ $# -gt 1 ]]; then
  echo "usage: $0 [build-directory]" >&2
  exit 2
fi

BUILD_DIR=${1:-${BUILD_DIR:-build}}
: "${QEMU_RISCV64:?Set QEMU_RISCV64 to the qemu-riscv64 executable}"
: "${QEMU_INSN_PLUGIN:?Set QEMU_INSN_PLUGIN to the QEMU libinsn.so plugin}"

VLEN=${VLEN:-128}
DEFAULT_MATCHES="vbcompress.vx vbcompress.vv vbexpand.vx vbexpand.vv vsetvli vle32.v vse32.v vand.vx vor.vv vsll.vi vsrl.vi"
MATCHES=${QEMU_INSN_MATCHES:-${DEFAULT_MATCHES}}

if [[ ! -x ${QEMU_RISCV64} ]]; then
  echo "error: QEMU_RISCV64 is not executable: ${QEMU_RISCV64}" >&2
  exit 1
fi
if [[ ! -f ${QEMU_INSN_PLUGIN} ]]; then
  echo "error: QEMU_INSN_PLUGIN does not exist: ${QEMU_INSN_PLUGIN}" >&2
  exit 1
fi

declare -a names=(
  "scalar"
  "rvv"
  "zvbext"
  "p3104"
  "p3104-zvbext"
)
declare -a executables=(
  "run-scalar"
  "run-rvv"
  "run-zvbext"
  "run-p3104"
  "run-p3104-zvbext"
)
declare -a cpus=(
  "max,v=false,zvbext=false"
  "max,v=true,vlen=${VLEN},zvbext=false"
  "max,v=true,vlen=${VLEN},zvbext=true"
  "max,v=false,zvbext=false"
  "max,v=true,vlen=${VLEN},zvbext=true"
)

match_arguments=""
for match in ${MATCHES}; do
  match_arguments+=",match=${match}"
done

if [[ ${QEMU_INSN_MATCHES+x} != x ]]; then
  for required in vbcompress.vx vbcompress.vv vbexpand.vx vbexpand.vv; do
    log_file=$(mktemp)
    trap 'rm -f "${log_file}"' EXIT
    "${QEMU_RISCV64}" \
      -cpu "max,v=true,vlen=${VLEN},zvbext=true" \
      -plugin "${QEMU_INSN_PLUGIN},inline=true,match=${required}" \
      -d plugin \
      "${BUILD_DIR}/run-p3104-zvbext" \
      >/dev/null 2>"${log_file}"
    hits=$(awk -v needle="${required}" '
      $1 == "Match:" {
        name = $2
        sub(/,$/, "", name)
        if (name == needle) { print $4; found = 1; exit }
      }
      END { if (!found) print 0 }
    ' "${log_file}")
    if [[ ${hits} -eq 0 ]]; then
      echo "error: run-p3104-zvbext executed no ${required} instructions" >&2
      rm -f "${log_file}"
      exit 1
    fi
    rm -f "${log_file}"
    trap - EXIT
  done
  echo "P3104R0 Zvbext instruction-form checks passed"
fi

printf "%-10s %15s" "variant" "total-insns"
for match in ${MATCHES}; do
  printf " %12s" "${match}"
done
printf "\n"

for index in "${!names[@]}"; do
  log_file=$(mktemp)
  trap 'rm -f "${log_file}"' EXIT

  plugin_arguments="inline=true"
  if [[ ${names[index]} != scalar && ${names[index]} != p3104 ]]; then
    plugin_arguments+="${match_arguments}"
  fi

  "${QEMU_RISCV64}" \
    -cpu "${cpus[index]}" \
    -plugin "${QEMU_INSN_PLUGIN},${plugin_arguments}" \
    -d plugin \
    "${BUILD_DIR}/${executables[index]}" \
    >/dev/null 2>"${log_file}"

  total=$(awk '/^total insns:/ {print $3}' "${log_file}")
  if [[ -z ${total} ]]; then
    echo "error: QEMU instruction plugin did not report a total" >&2
    cat "${log_file}" >&2
    exit 1
  fi

  printf "%-10s %15s" "${names[index]}" "${total}"
  for match in ${MATCHES}; do
    hits=0
    if [[ ${names[index]} != scalar && ${names[index]} != p3104 ]]; then
      hits=$(awk -v needle="${match}" '
        $1 == "Match:" {
          name = $2
          sub(/,$/, "", name)
          if (name == needle) {
            print $4
            found = 1
            exit
          }
        }
        END {
          if (!found)
            print 0
        }
      ' "${log_file}")
    fi
    printf " %12s" "${hits}"
  done
  printf "\n"

  rm -f "${log_file}"
  trap - EXIT
done
