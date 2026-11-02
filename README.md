# Zvbce Application Examples

This project provides executable C examples for three workloads from the
Zvbce proposal gist and one LLM tokenizer workload:

- packing and expanding four 6-bit Base64 fields;
- spreading and gathering four 7-bit ULEB128 payloads;
- interleaving and deinterleaving two 16-bit Morton/Z-order coordinates;
- encoding and decoding classified 1/2/3/4-byte UTF-8 payloads used by
  tokenizer frontends.

It also includes a C++20 compatibility implementation of
[WG21 P3104R0, Bit permutations](https://wg21.link/p3104r0), together with
executable examples for all eight proposed operations.

## Project Layout

- `applications.h`: common interface implemented by all variants;
- `run.c`: the single test driver shared by all variants;
- `reference.c`: scalar C reference implementation;
- `rvv_without_zvbce.c`: RVV implementation using shifts and Boolean
  operations;
- `rvv_with_zvbce.c`: RVV implementation using `vbcompress` and `vbexpand`;
- `CMakeLists.txt`: builds the three original C executables and two P3104R0
  C++ executables;
- `cmake/riscv64-toolchain.cmake`: environment-driven cross-compilation file;
- `scripts/run-qemu.sh`: runs all executables under QEMU;
- `scripts/count-instructions.sh`: counts dynamic instructions with QEMU's
  `libinsn.so` plugin;
- `TOKENIZERS.md`: source-backed analysis of Hugging Face Tokenizers;
- `p3104/bit_permutations.hpp`: portable C++20 implementation of the eight
  functions proposed by P3104R0;
- `p3104/examples.cpp`: exhaustive small-type tests plus application examples
  for bitmap mirroring, Morton/Z-order indexing, UTF-8 payload reshaping, and
  fixed-size subset enumeration;
- `p3104/batch.hpp`: common batched interface for shared and per-element masks;
- `p3104/batch_portable.cpp`: scalar batched adapter built from the P3104R0
  functions;
- `p3104/batch_zvbce.cpp`: explicit RVV implementation of the same batched
  interface using `vbcompress.vx`/`vbexpand.vx` for shared masks and
  `vbcompress.vv`/`vbexpand.vv` for per-element masks.

The three C implementations export the same function names, and their driver
contains no implementation-selection preprocessor macros. The two C++
executables similarly share one P3104R0 test/application driver while CMake
links either the portable or the explicit Zvbce batch backend.

The P3104R0 compatibility functions live in namespace `p3104r0`. Applications
must not add them to `std`; doing so before the standard library provides the
final facility would be undefined behavior. The proposal's feature-test macro
value is intentionally not emulated because R0 specifies it only as the
placeholder `20XXXXL`.

## P3104R0 Interface and Applications

The compatibility header implements the exact R0 function set:

```cpp
bit_reverse(x)
bit_repeat(x, length)
next_bit_permutation(x)
prev_bit_permutation(x)
bit_compressr(x, mask)
bit_expandr(x, mask)
bit_compressl(x, mask)
bit_expandl(x, mask)
```

All functions are `constexpr` and constrained to unsigned integer types other
than `bool`. As specified by R0, `bit_repeat` requires `length > 0` and is the
only function above that is not declared `noexcept`.

R0's worked `bit_reverse(uint32_t{0x00001234})` example prints
`0x24c80000`, but its normative formula and its own illustrative loop both
produce `0x2c480000`. This implementation follows the specified full-width bit
reversal semantics (`0x2c480000`) and tests that result.

The examples connect the primitives to concrete workloads:

| Operation | Example | Why it is useful |
|---|---|---|
| `bit_reverse` | 1-bit-per-pixel bitmap mirroring | Reverses a full scanline without per-pixel indexing |
| `bit_repeat` | Morton even/odd masks | Expresses `0x55555555` and `0xaaaaaaaa` without magic constants |
| `bit_compressr` / `bit_expandr` | Morton/Z-order and UTF-8 payloads | Gathers or scatters non-contiguous bits while preserving order |
| `next_bit_permutation` / `prev_bit_permutation` | Fixed-size subsets | Enumerates all bitsets with a fixed population count |
| left-oriented compress/expand | Full-domain differential tests | Supports formats whose packed field is aligned to the most-significant edge |

Other potential uses identified by the proposal include CRC processing, FFT
bit-reversed addressing, recurring debug/test patterns, chess bitboards, and
genomics. The same primitives can also express protocol bit-field extraction,
bitmap-index compaction, fixed-Hamming-weight fuzzing, and sparse hardware
register layouts. Those latter examples are engineering extrapolations rather
than claims made directly by P3104R0.

Each P3104 executable exhaustively checks every `uint8_t` input for
reverse/repeat/next/previous and all 65,536 `(value, mask)` pairs for each
compress/expand direction. It also tests zero-length, tail-length, and in-place
batch calls before running the larger application examples.

On Clang versions that provide `__builtin_elementwise_pext` and
`__builtin_elementwise_pdep`, the scalar compatibility functions use those
builtins. This keeps constant evaluation available and exposes the semantic
operations to the optimizer. The Zvbce executable additionally links an
explicit batched backend: Morton uses the `.vx` forms because every element
shares an interleave mask, while UTF-8 uses the `.vv` forms because each code
point has a length-dependent payload mask. The portable executable links the
same applications to scalar loops instead.

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
* LLVM: https://github.com/wangpc-pp/llvm-project/tree/main-riscv-zvbce
* QEMU: https://github.com/wangpc-pp/qemu/tree/master-zvbce

Set the following environment variables:

| Variable | Required | Description |
|---|---|---|
| `RISCV_CLANG` | yes | Clang executable with Zvbce support |
| `RISCV_CLANGXX` | optional | Clang++ executable; defaults to `clang++` next to `RISCV_CLANG` |
| `RISCV_SYSROOT` | yes | RISC-V Linux sysroot |
| `RISCV_GCC_TOOLCHAIN` | optional | GCC toolchain root used by Clang for runtime objects and libraries |
| `QEMU_RISCV64` | for execution | `qemu-riscv64` executable with Zvbce support |
| `QEMU_INSN_PLUGIN` | for counting | QEMU `libinsn.so` plugin |
| `BUILD_DIR` | optional | Build directory; defaults to `build` |
| `BUILD_JOBS` | optional | Parallel build jobs |
| `VLENS` | optional | Space-separated VLEN values for `run-qemu.sh`; defaults to `128 256 512` |
| `VLEN` | optional | VLEN used by instruction counting; defaults to `128` |
| `QEMU_INSN_MATCHES` | optional | Space-separated instruction prefixes counted by the plugin script |

Example environment setup:

```sh
export RISCV_CLANG=/path/to/clang
export RISCV_CLANGXX=/path/to/clang++
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
- `build/run-zvbce`
- `build/run-p3104`
- `build/run-p3104-zvbce`

Static linking is enabled by default. Disable it when using a dynamic sysroot:

```sh
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/riscv64-toolchain.cmake \
  -DZVBCE_APPLICATIONS_STATIC_LINK=OFF
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

The script runs the scalar and portable P3104 implementations once, runs the
vector implementations and P3104 Zvbce build at VLEN 128/256/512, and verifies
that both Zvbce binaries fail when the extension is disabled.

The three original C implementations must produce these checksums:

```text
Base64 reshape:   f6e12d313365e693
ULEB128 payloads: f8c9c4d4aa5469ef
Morton 2D:        fb4d798cd8c9a83f
Tokenizer UTF-8:  229c5e13cd63a314
```

Both P3104R0 backends must produce these application checksums:

```text
Morton/Z-order:   73fafc9799b5b0ad
UTF-8 payloads:   fd44b68e0429510c
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
hit counts for all five binaries. With the default match set it additionally
requires the P3104R0 Zvbce executable to execute each of `vbcompress.vx`,
`vbcompress.vv`, `vbexpand.vx`, and `vbexpand.vv`. Override the reporting
prefixes with `QEMU_INSN_MATCHES`; doing so disables this default four-form
gate.

The Zvbce kernels contain `vbcompress.vx`/`vbexpand.vx` and
`vbcompress.vv`/`vbexpand.vv`. The plain RVV kernels expand the same work into
multiple `vsll`, `vsrl`, `vand`, and `vor` operations.

The following loop-body counts were obtained from optimized assembly and
include `vsetvli`, memory operations, pointer updates, and branches:

| Scenario | Plain RVV | Zvbce RVV |
|---|---:|---:|
| Base64 decode reshape | 18 | 9 |
| Base64 encode reshape | 18 | 9 |
| ULEB128 payload spread | 18 | 9 |
| ULEB128 payload gather | 18 | 9 |
| Morton 2D encode | 38 | 13 |
| Morton 2D decode | 37 | 12 |
| Tokenizer UTF-8 payload decode | 28 | 11 |
| Tokenizer UTF-8 payload encode | 33 | 14 |
