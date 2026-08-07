# Zvbext Application Examples

This project provides executable C examples for three workloads from the
Zvbext proposal gist and one LLM tokenizer workload:

- packing and expanding four 6-bit Base64 fields;
- spreading and gathering four 7-bit ULEB128 payloads;
- interleaving and deinterleaving two 16-bit Morton/Z-order coordinates;
- encoding and decoding classified 1/2/3/4-byte UTF-8 payloads used by
  tokenizer frontends.

## Project Layout

- `applications.h`: common interface implemented by all variants;
- `run.c`: the single test driver shared by all variants;
- `reference.c`: scalar C reference implementation;
- `rvv_without_zvbext.c`: RVV implementation using shifts and Boolean
  operations;
- `rvv_with_zvbext.c`: RVV implementation using `vbcompress` and `vbexpand`;
- `CMakeLists.txt`: builds the three executables;
- `cmake/riscv64-toolchain.cmake`: environment-driven cross-compilation file;
- `scripts/run-qemu.sh`: runs all executables under QEMU;
- `scripts/count-instructions.sh`: counts dynamic instructions with QEMU's
  `libinsn.so` plugin;
- `TOKENIZERS.md`: source-backed analysis of Hugging Face Tokenizers.

All three implementations export the same function names. The driver contains
no implementation-selection preprocessor macros; CMake selects the
implementation by linking a different source file.

## Scope

The examples isolate the bit-reshape kernels from the rest of each format
pipeline:

- Base64 excludes alphabet lookup, validation, and the final byte shuffle.
- ULEB128 excludes continuation-bit validation and variable-length compact
  stores.
- Morton covers the complete coordinate interleave/deinterleave core.
- The tokenizer example assumes UTF-8 validation and character-length
  classification have already completed. It measures payload gather/scatter,
  not BPE merge, vocabulary hashing, or priority queues.

`vbcompress` is an element-wise bit-compress operation. It must not be confused
with `vcompress.vm`, which compacts whole vector elements.

## Prerequisites

PoC softwares:
* LLVM: https://github.com/wangpc-pp/llvm-project/tree/main-riscv-zvbext
* QEMU: https://github.com/wangpc-pp/qemu/tree/master-zvbext

Set the following environment variables:

| Variable | Required | Description |
|---|---|---|
| `RISCV_CLANG` | yes | Clang executable with Zvbext support |
| `RISCV_SYSROOT` | yes | RISC-V Linux sysroot |
| `RISCV_GCC_TOOLCHAIN` | optional | GCC toolchain root used by Clang for runtime objects and libraries |
| `QEMU_RISCV64` | for execution | `qemu-riscv64` executable with Zvbext support |
| `QEMU_INSN_PLUGIN` | for counting | QEMU `libinsn.so` plugin |
| `BUILD_DIR` | optional | Build directory; defaults to `build` |
| `BUILD_JOBS` | optional | Parallel build jobs |
| `VLENS` | optional | Space-separated VLEN values for `run-qemu.sh`; defaults to `128 256 512` |
| `VLEN` | optional | VLEN used by instruction counting; defaults to `128` |
| `QEMU_INSN_MATCHES` | optional | Space-separated instruction prefixes counted by the plugin script |

Example environment setup:

```sh
export RISCV_CLANG=/path/to/clang
export RISCV_SYSROOT=/path/to/riscv/sysroot
export RISCV_GCC_TOOLCHAIN=/path/to/riscv/toolchain
export QEMU_RISCV64=/path/to/qemu-riscv64
export QEMU_INSN_PLUGIN=/path/to/libinsn.so
```

No source file or script contains a machine-specific tool path.

## CMake Build

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The build creates:

- `build/run-scalar`
- `build/run-rvv`
- `build/run-zvbext`

Static linking is enabled by default. Disable it when using a dynamic sysroot:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64-toolchain.cmake \
  -DZVBEXT_APPLICATIONS_STATIC_LINK=OFF
```

## Run Under QEMU

Build and run in one command:

```sh
./run.sh
```

Or run an existing build:

```sh
scripts/run-qemu.sh build
```

The script runs the scalar implementation once, runs both vector
implementations at VLEN 128/256/512, and verifies that the Zvbext binary fails
when the extension is disabled.

All implementations must produce these checksums:

```text
Base64 reshape:   f6e12d313365e693
ULEB128 payloads: f8c9c4d4aa5469ef
Morton 2D:        fb4d798cd8c9a83f
Tokenizer UTF-8:  229c5e13cd63a314
```

## Dynamic Instruction Counting

Use QEMU's instruction plugin after building:

```sh
scripts/count-instructions.sh build
```

Or through CMake:

```sh
cmake --build build --target instruction-count
```

The script reports total dynamic instructions and selected instruction-prefix
hit counts for the scalar, RVV, and Zvbext binaries. Override the selected
prefixes with `QEMU_INSN_MATCHES`.

The Zvbext kernels contain `vbcompress.vx`/`vbexpand.vx` and
`vbcompress.vv`/`vbexpand.vv`. The plain RVV kernels expand the same work into
multiple `vsll`, `vsrl`, `vand`, and `vor` operations.

The following loop-body counts were obtained from optimized assembly and
include `vsetvli`, memory operations, pointer updates, and branches:

| Scenario | Plain RVV | Zvbext RVV |
|---|---:|---:|
| Base64 decode reshape | 18 | 9 |
| Base64 encode reshape | 18 | 9 |
| ULEB128 payload spread | 18 | 9 |
| ULEB128 payload gather | 18 | 9 |
| Morton 2D encode | 38 | 13 |
| Morton 2D decode | 37 | 12 |
| Tokenizer UTF-8 payload decode | 28 | 11 |
| Tokenizer UTF-8 payload encode | 33 | 14 |
