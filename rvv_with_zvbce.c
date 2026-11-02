#include <riscv_vector.h>
#include <stddef.h>
#include <stdint.h>

#include "applications.h"

/*
 * Zvbce performs the same per-element bit reshapes directly.
 *
 * Constant masks select the .vx forms, so a single GPR mask is shared by all
 * active elements. The surrounding load/store loop is unchanged.
 */

const char *implementation_name(void) { return "RVV + Zvbce"; }

void base64_decode_reshape(const uint32_t *input, uint32_t *output,
                           size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t words = __riscv_vle32_v_u32m1(input, vl);
    vuint32m1_t payloads =
        __riscv_vbcompress_vx_u32m1(words, UINT32_C(0x3f3f3f3f), vl);
    __riscv_vse32_v_u32m1(output, payloads, vl);
    input += vl;
    output += vl;
    count -= vl;
  }
}

void base64_encode_reshape(const uint32_t *input, uint32_t *output,
                           size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t payloads = __riscv_vle32_v_u32m1(input, vl);
    vuint32m1_t words =
        __riscv_vbexpand_vx_u32m1(payloads, UINT32_C(0x3f3f3f3f), vl);
    __riscv_vse32_v_u32m1(output, words, vl);
    input += vl;
    output += vl;
    count -= vl;
  }
}

void uleb28_encode_payloads(const uint32_t *input, uint32_t *output,
                            size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t values = __riscv_vle32_v_u32m1(input, vl);
    vuint32m1_t slots =
        __riscv_vbexpand_vx_u32m1(values, UINT32_C(0x7f7f7f7f), vl);
    __riscv_vse32_v_u32m1(output, slots, vl);
    input += vl;
    output += vl;
    count -= vl;
  }
}

void uleb28_decode_payloads(const uint32_t *input, uint32_t *output,
                            size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t slots = __riscv_vle32_v_u32m1(input, vl);
    vuint32m1_t values =
        __riscv_vbcompress_vx_u32m1(slots, UINT32_C(0x7f7f7f7f), vl);
    __riscv_vse32_v_u32m1(output, values, vl);
    input += vl;
    output += vl;
    count -= vl;
  }
}

void morton2d_encode(const uint32_t *x, const uint32_t *y, uint32_t *output,
                     size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t xs = __riscv_vle32_v_u32m1(x, vl);
    vuint32m1_t ys = __riscv_vle32_v_u32m1(y, vl);
    xs = __riscv_vbexpand_vx_u32m1(xs, UINT32_C(0x55555555), vl);
    ys = __riscv_vbexpand_vx_u32m1(ys, UINT32_C(0xaaaaaaaa), vl);
    vuint32m1_t codes = __riscv_vor_vv_u32m1(xs, ys, vl);
    __riscv_vse32_v_u32m1(output, codes, vl);
    x += vl;
    y += vl;
    output += vl;
    count -= vl;
  }
}

void morton2d_decode(const uint32_t *input, uint32_t *x, uint32_t *y,
                     size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t codes = __riscv_vle32_v_u32m1(input, vl);
    vuint32m1_t xs =
        __riscv_vbcompress_vx_u32m1(codes, UINT32_C(0x55555555), vl);
    vuint32m1_t ys =
        __riscv_vbcompress_vx_u32m1(codes, UINT32_C(0xaaaaaaaa), vl);
    __riscv_vse32_v_u32m1(x, xs, vl);
    __riscv_vse32_v_u32m1(y, ys, vl);
    input += vl;
    x += vl;
    y += vl;
    count -= vl;
  }
}

void tokenizer_utf8_decode(const uint32_t *packed,
                           const uint32_t *payload_masks, uint32_t *codepoints,
                           size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t packed_values = __riscv_vle32_v_u32m1(packed, vl);
    vuint32m1_t masks = __riscv_vle32_v_u32m1(payload_masks, vl);
    vuint32m1_t decoded = __riscv_vbcompress_vv_u32m1(packed_values, masks, vl);
    __riscv_vse32_v_u32m1(codepoints, decoded, vl);
    packed += vl;
    payload_masks += vl;
    codepoints += vl;
    count -= vl;
  }
}

void tokenizer_utf8_encode(const uint32_t *codepoints,
                           const uint32_t *payload_masks,
                           const uint32_t *prefixes, uint32_t *packed,
                           size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t values = __riscv_vle32_v_u32m1(codepoints, vl);
    vuint32m1_t masks = __riscv_vle32_v_u32m1(payload_masks, vl);
    vuint32m1_t prefix_values = __riscv_vle32_v_u32m1(prefixes, vl);
    vuint32m1_t encoded = __riscv_vbexpand_vv_u32m1(values, masks, vl);
    encoded = __riscv_vor_vv_u32m1(encoded, prefix_values, vl);
    __riscv_vse32_v_u32m1(packed, encoded, vl);
    codepoints += vl;
    payload_masks += vl;
    prefixes += vl;
    packed += vl;
    count -= vl;
  }
}
