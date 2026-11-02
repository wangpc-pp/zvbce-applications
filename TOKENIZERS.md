# Hugging Face Tokenizers and Zvbce

This analysis is based on Hugging Face `tokenizers` main commit
`447890f84deab25794ccfdee2a877901b6893569`.

## Conclusion

Zvbce can help tokenizer implementations, but its direct use is limited to
element-wise bit gather/scatter in UTF-8 and ByteLevel frontend/backend code.
It does not directly accelerate BPE merging, vocabulary hashing, or priority
queues.

A suitable integration boundary is:

1. validate UTF-8 and classify characters as 1/2/3/4 bytes;
2. place each character in a 32-bit lane with the final continuation byte in
   the least significant byte;
3. use `vbcompress.vv` with a per-lane payload mask to extract Unicode code
   points;
4. use `vbexpand.vv` with a per-lane payload mask, then OR fixed UTF-8 prefix
   bits to encode code points.

`tokenizer_utf8_decode` and `tokenizer_utf8_encode` implement this kernel. The
common driver mixes 1/2/3/4-byte characters and verifies that all three
implementations produce identical results.

## Relevance to Hugging Face Tokenizers

The `ByteLevel` pre-tokenizer walks `char_indices()`, reads the UTF-8 bytes for
each character, and maps every byte to the GPT-2 byte alphabet:

- `tokenizers/src/pre_tokenizers/byte_level.rs:132`
- `tokenizers/src/pre_tokenizers/byte_level.rs:135`
- `tokenizers/src/pre_tokenizers/byte_level.rs:138`

The `ByteLevel` decoder maps token characters back to original bytes and
finishes with `String::from_utf8_lossy`:

- `tokenizers/src/pre_tokenizers/byte_level.rs:155`
- `tokenizers/src/pre_tokenizers/byte_level.rs:160`
- `tokenizers/src/pre_tokenizers/byte_level.rs:170`

Other model implementations repeatedly depend on UTF-8 character boundaries:

- BPE uses `char_indices()` to construct initial symbols:
  `tokenizers/src/models/bpe/model.rs:465`
- WordPiece shortens failed candidates with `len_utf8()`:
  `tokenizers/src/models/wordpiece/mod.rs:245`
- Unigram advances its lattice by the first character's `len_utf8()`:
  `tokenizers/src/models/unigram/model.rs:175`

If a tokenizer adds a batched UTF-8 classification/decoding frontend, Zvbce
can shorten the bit-reshape stage from classified characters to code points
and from code points to UTF-8 payloads.

## Why It Does Not Accelerate the Whole BPE Algorithm

The BPE merge path primarily:

- looks up symbol pairs in an `AHashMap`;
- selects candidates by merge rank using a quaternary heap;
- updates linked-symbol indices and inserts newly formed pairs.

Relevant source locations include:

- `tokenizers/src/models/bpe/word.rs:162`
- `tokenizers/src/models/bpe/word.rs:170`
- `tokenizers/src/models/bpe/word.rs:180`
- `tokenizers/src/models/bpe/word.rs:198`
- `tokenizers/src/models/bpe/word.rs:217`

These operations are hashing, indirect memory accesses, priority-queue
operations, and control flow rather than regular bit compress/expand.
Removing merged symbols is cross-element compaction; a vector implementation
would consider `vcompress.vm`, not element-wise `vbcompress`.

## UTF-8 Lane Layout

Each character is stored in a 32-bit lane with its final continuation byte in
the least significant byte:

| UTF-8 length | Payload mask | Fixed prefix |
|---|---:|---:|
| 1 byte | `0x0000007f` | `0x00000000` |
| 2 bytes | `0x00001f3f` | `0x0000c080` |
| 3 bytes | `0x000f3f3f` | `0x00e08080` |
| 4 bytes | `0x073f3f3f` | `0xf0808080` |

For example, U+4F60 is encoded as `E4 BD A0` and stored as `0x00E4BDA0`:

```text
vbcompress(0x00e4bda0, 0x000f3f3f) = 0x00004f60
vbexpand(0x00004f60, 0x000f3f3f) | 0x00e08080 = 0x00e4bda0
```

## Assembly Comparison

With an optimized Clang 24 build:

| Kernel | Plain RVV loop | Zvbce loop |
|---|---:|---:|
| UTF-8 payload decode | 28 dynamic instructions | 11 dynamic instructions |
| UTF-8 payload encode | 33 dynamic instructions | 14 dynamic instructions |

The Zvbce decode core is:

```asm
vle32.v        v8, (packed)
vle32.v        v9, (payload_masks)
vbcompress.vv  v8, v8, v9
```

The encode core is:

```asm
vle32.v      v8, (codepoints)
vle32.v      v9, (payload_masks)
vbexpand.vv  v8, v8, v9
vle32.v      v9, (prefixes)
vor.vv       v8, v8, v9
```

The plain RVV implementation computes all four layouts and selects among them
with three `vmseq.vx` operations and masked `vor.vv` instructions.

## Expected Benefit Boundaries

- Multilingual, CJK, and emoji-heavy input contains more multi-byte UTF-8 and
  is more likely to benefit.
- ASCII-heavy English input needs no bit compression for ASCII lanes, so the
  benefit may be small. A practical implementation should retain a bulk ASCII
  fast path.
- Current Hugging Face `tokenizers` code has no batched RVV UTF-8 frontend.
  The loop instruction counts above are not an end-to-end tokenizer speedup.
- A real integration should profile ByteLevel processing, offset conversion,
  and model merging separately, then account for classification and data
  rearrangement costs.
