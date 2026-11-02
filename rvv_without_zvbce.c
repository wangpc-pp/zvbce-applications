#include <riscv_vector.h>
#include <stddef.h>
#include <stdint.h>

#include "applications.h"

/*
 * These routines implement the key per-element bit reshapes with ordinary
 * RVV shifts and Boolean operations. The masks are compile-time constants,
 * so they are already much cheaper than fully general software pext/pdep.
 *
 * They intentionally do not use vcompress.vm: vcompress.vm moves whole vector
 * elements, while these algorithms move selected bits within each element.
 */

const char *implementation_name(void) { return "RVV without Zvbce"; }

static vuint32m1_t base64_pack6_without_zvbce(vuint32m1_t reversed_words,
                                              size_t vl) {
  vuint32m1_t field0 =
      __riscv_vand_vx_u32m1(reversed_words, UINT32_C(0x0000003f), vl);
  vuint32m1_t field1 = __riscv_vsrl_vx_u32m1(reversed_words, 2, vl);
  field1 = __riscv_vand_vx_u32m1(field1, UINT32_C(0x00000fc0), vl);
  vuint32m1_t field2 = __riscv_vsrl_vx_u32m1(reversed_words, 4, vl);
  field2 = __riscv_vand_vx_u32m1(field2, UINT32_C(0x0003f000), vl);
  vuint32m1_t field3 = __riscv_vsrl_vx_u32m1(reversed_words, 6, vl);
  field3 = __riscv_vand_vx_u32m1(field3, UINT32_C(0x00fc0000), vl);

  vuint32m1_t result = __riscv_vor_vv_u32m1(field0, field1, vl);
  result = __riscv_vor_vv_u32m1(result, field2, vl);
  return __riscv_vor_vv_u32m1(result, field3, vl);
}

static vuint32m1_t base64_spread6_without_zvbce(vuint32m1_t payloads,
                                                size_t vl) {
  vuint32m1_t field0 =
      __riscv_vand_vx_u32m1(payloads, UINT32_C(0x0000003f), vl);
  vuint32m1_t field1 =
      __riscv_vand_vx_u32m1(payloads, UINT32_C(0x00000fc0), vl);
  field1 = __riscv_vsll_vx_u32m1(field1, 2, vl);
  vuint32m1_t field2 =
      __riscv_vand_vx_u32m1(payloads, UINT32_C(0x0003f000), vl);
  field2 = __riscv_vsll_vx_u32m1(field2, 4, vl);
  vuint32m1_t field3 =
      __riscv_vand_vx_u32m1(payloads, UINT32_C(0x00fc0000), vl);
  field3 = __riscv_vsll_vx_u32m1(field3, 6, vl);

  vuint32m1_t result = __riscv_vor_vv_u32m1(field0, field1, vl);
  result = __riscv_vor_vv_u32m1(result, field2, vl);
  return __riscv_vor_vv_u32m1(result, field3, vl);
}

static vuint32m1_t uleb28_spread_without_zvbce(vuint32m1_t values, size_t vl) {
  vuint32m1_t byte0 = __riscv_vand_vx_u32m1(values, UINT32_C(0x0000007f), vl);
  vuint32m1_t byte1 = __riscv_vand_vx_u32m1(values, UINT32_C(0x00003f80), vl);
  byte1 = __riscv_vsll_vx_u32m1(byte1, 1, vl);
  vuint32m1_t byte2 = __riscv_vand_vx_u32m1(values, UINT32_C(0x001fc000), vl);
  byte2 = __riscv_vsll_vx_u32m1(byte2, 2, vl);
  vuint32m1_t byte3 = __riscv_vand_vx_u32m1(values, UINT32_C(0x0fe00000), vl);
  byte3 = __riscv_vsll_vx_u32m1(byte3, 3, vl);

  vuint32m1_t result = __riscv_vor_vv_u32m1(byte0, byte1, vl);
  result = __riscv_vor_vv_u32m1(result, byte2, vl);
  return __riscv_vor_vv_u32m1(result, byte3, vl);
}

static vuint32m1_t uleb28_gather_without_zvbce(vuint32m1_t slots, size_t vl) {
  vuint32m1_t byte0 = __riscv_vand_vx_u32m1(slots, UINT32_C(0x0000007f), vl);
  vuint32m1_t byte1 = __riscv_vsrl_vx_u32m1(slots, 1, vl);
  byte1 = __riscv_vand_vx_u32m1(byte1, UINT32_C(0x00003f80), vl);
  vuint32m1_t byte2 = __riscv_vsrl_vx_u32m1(slots, 2, vl);
  byte2 = __riscv_vand_vx_u32m1(byte2, UINT32_C(0x001fc000), vl);
  vuint32m1_t byte3 = __riscv_vsrl_vx_u32m1(slots, 3, vl);
  byte3 = __riscv_vand_vx_u32m1(byte3, UINT32_C(0x0fe00000), vl);

  vuint32m1_t result = __riscv_vor_vv_u32m1(byte0, byte1, vl);
  result = __riscv_vor_vv_u32m1(result, byte2, vl);
  return __riscv_vor_vv_u32m1(result, byte3, vl);
}

static vuint32m1_t morton_spread16_without_zvbce(vuint32m1_t values,
                                                 size_t vl) {
  values = __riscv_vand_vx_u32m1(values, UINT32_C(0x0000ffff), vl);

  vuint32m1_t shifted = __riscv_vsll_vx_u32m1(values, 8, vl);
  values = __riscv_vor_vv_u32m1(values, shifted, vl);
  values = __riscv_vand_vx_u32m1(values, UINT32_C(0x00ff00ff), vl);

  shifted = __riscv_vsll_vx_u32m1(values, 4, vl);
  values = __riscv_vor_vv_u32m1(values, shifted, vl);
  values = __riscv_vand_vx_u32m1(values, UINT32_C(0x0f0f0f0f), vl);

  shifted = __riscv_vsll_vx_u32m1(values, 2, vl);
  values = __riscv_vor_vv_u32m1(values, shifted, vl);
  values = __riscv_vand_vx_u32m1(values, UINT32_C(0x33333333), vl);

  shifted = __riscv_vsll_vx_u32m1(values, 1, vl);
  values = __riscv_vor_vv_u32m1(values, shifted, vl);
  return __riscv_vand_vx_u32m1(values, UINT32_C(0x55555555), vl);
}

static vuint32m1_t morton_compact16_without_zvbce(vuint32m1_t values,
                                                  size_t vl) {
  values = __riscv_vand_vx_u32m1(values, UINT32_C(0x55555555), vl);

  vuint32m1_t shifted = __riscv_vsrl_vx_u32m1(values, 1, vl);
  values = __riscv_vor_vv_u32m1(values, shifted, vl);
  values = __riscv_vand_vx_u32m1(values, UINT32_C(0x33333333), vl);

  shifted = __riscv_vsrl_vx_u32m1(values, 2, vl);
  values = __riscv_vor_vv_u32m1(values, shifted, vl);
  values = __riscv_vand_vx_u32m1(values, UINT32_C(0x0f0f0f0f), vl);

  shifted = __riscv_vsrl_vx_u32m1(values, 4, vl);
  values = __riscv_vor_vv_u32m1(values, shifted, vl);
  values = __riscv_vand_vx_u32m1(values, UINT32_C(0x00ff00ff), vl);

  shifted = __riscv_vsrl_vx_u32m1(values, 8, vl);
  values = __riscv_vor_vv_u32m1(values, shifted, vl);
  return __riscv_vand_vx_u32m1(values, UINT32_C(0x0000ffff), vl);
}

void base64_decode_reshape(const uint32_t *input, uint32_t *output,
                           size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t words = __riscv_vle32_v_u32m1(input, vl);
    vuint32m1_t payloads = base64_pack6_without_zvbce(words, vl);
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
    vuint32m1_t words = base64_spread6_without_zvbce(payloads, vl);
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
    vuint32m1_t slots = uleb28_spread_without_zvbce(values, vl);
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
    vuint32m1_t values = uleb28_gather_without_zvbce(slots, vl);
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
    xs = morton_spread16_without_zvbce(xs, vl);
    ys = morton_spread16_without_zvbce(ys, vl);
    ys = __riscv_vsll_vx_u32m1(ys, 1, vl);
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
    vuint32m1_t xs = morton_compact16_without_zvbce(codes, vl);
    codes = __riscv_vsrl_vx_u32m1(codes, 1, vl);
    vuint32m1_t ys = morton_compact16_without_zvbce(codes, vl);
    __riscv_vse32_v_u32m1(x, xs, vl);
    __riscv_vse32_v_u32m1(y, ys, vl);
    input += vl;
    x += vl;
    y += vl;
    count -= vl;
  }
}

static vuint32m1_t tokenizer_utf8_decode_2(vuint32m1_t packed, size_t vl) {
  vuint32m1_t field0 = __riscv_vand_vx_u32m1(packed, UINT32_C(0x0000003f), vl);
  vuint32m1_t field1 = __riscv_vsrl_vx_u32m1(packed, 2, vl);
  field1 = __riscv_vand_vx_u32m1(field1, UINT32_C(0x000007c0), vl);
  return __riscv_vor_vv_u32m1(field0, field1, vl);
}

static vuint32m1_t tokenizer_utf8_decode_3(vuint32m1_t packed, size_t vl) {
  vuint32m1_t field0 = __riscv_vand_vx_u32m1(packed, UINT32_C(0x0000003f), vl);
  vuint32m1_t field1 = __riscv_vsrl_vx_u32m1(packed, 2, vl);
  field1 = __riscv_vand_vx_u32m1(field1, UINT32_C(0x00000fc0), vl);
  vuint32m1_t field2 = __riscv_vsrl_vx_u32m1(packed, 4, vl);
  field2 = __riscv_vand_vx_u32m1(field2, UINT32_C(0x0000f000), vl);
  vuint32m1_t result = __riscv_vor_vv_u32m1(field0, field1, vl);
  return __riscv_vor_vv_u32m1(result, field2, vl);
}

static vuint32m1_t tokenizer_utf8_decode_4(vuint32m1_t packed, size_t vl) {
  vuint32m1_t field0 = __riscv_vand_vx_u32m1(packed, UINT32_C(0x0000003f), vl);
  vuint32m1_t field1 = __riscv_vsrl_vx_u32m1(packed, 2, vl);
  field1 = __riscv_vand_vx_u32m1(field1, UINT32_C(0x00000fc0), vl);
  vuint32m1_t field2 = __riscv_vsrl_vx_u32m1(packed, 4, vl);
  field2 = __riscv_vand_vx_u32m1(field2, UINT32_C(0x0003f000), vl);
  vuint32m1_t field3 = __riscv_vsrl_vx_u32m1(packed, 6, vl);
  field3 = __riscv_vand_vx_u32m1(field3, UINT32_C(0x001c0000), vl);
  vuint32m1_t result = __riscv_vor_vv_u32m1(field0, field1, vl);
  result = __riscv_vor_vv_u32m1(result, field2, vl);
  return __riscv_vor_vv_u32m1(result, field3, vl);
}

static vuint32m1_t tokenizer_utf8_encode_2(vuint32m1_t codepoints, size_t vl) {
  vuint32m1_t field0 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x0000003f), vl);
  vuint32m1_t field1 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x000007c0), vl);
  field1 = __riscv_vsll_vx_u32m1(field1, 2, vl);
  return __riscv_vor_vv_u32m1(field0, field1, vl);
}

static vuint32m1_t tokenizer_utf8_encode_3(vuint32m1_t codepoints, size_t vl) {
  vuint32m1_t field0 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x0000003f), vl);
  vuint32m1_t field1 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x00000fc0), vl);
  field1 = __riscv_vsll_vx_u32m1(field1, 2, vl);
  vuint32m1_t field2 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x0000f000), vl);
  field2 = __riscv_vsll_vx_u32m1(field2, 4, vl);
  vuint32m1_t result = __riscv_vor_vv_u32m1(field0, field1, vl);
  return __riscv_vor_vv_u32m1(result, field2, vl);
}

static vuint32m1_t tokenizer_utf8_encode_4(vuint32m1_t codepoints, size_t vl) {
  vuint32m1_t field0 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x0000003f), vl);
  vuint32m1_t field1 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x00000fc0), vl);
  field1 = __riscv_vsll_vx_u32m1(field1, 2, vl);
  vuint32m1_t field2 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x0003f000), vl);
  field2 = __riscv_vsll_vx_u32m1(field2, 4, vl);
  vuint32m1_t field3 =
      __riscv_vand_vx_u32m1(codepoints, UINT32_C(0x001c0000), vl);
  field3 = __riscv_vsll_vx_u32m1(field3, 6, vl);
  vuint32m1_t result = __riscv_vor_vv_u32m1(field0, field1, vl);
  result = __riscv_vor_vv_u32m1(result, field2, vl);
  return __riscv_vor_vv_u32m1(result, field3, vl);
}

void tokenizer_utf8_decode(const uint32_t *packed,
                           const uint32_t *payload_masks, uint32_t *codepoints,
                           size_t count) {
  while (count != 0) {
    size_t vl = __riscv_vsetvl_e32m1(count);
    vuint32m1_t packed_values = __riscv_vle32_v_u32m1(packed, vl);
    vuint32m1_t masks = __riscv_vle32_v_u32m1(payload_masks, vl);
    vuint32m1_t decoded =
        __riscv_vand_vx_u32m1(packed_values, UINT32_C(0x0000007f), vl);
    vuint32m1_t decoded2 = tokenizer_utf8_decode_2(packed_values, vl);
    vuint32m1_t decoded3 = tokenizer_utf8_decode_3(packed_values, vl);
    vuint32m1_t decoded4 = tokenizer_utf8_decode_4(packed_values, vl);
    vbool32_t is2 = __riscv_vmseq_vx_u32m1_b32(masks, UINT32_C(0x00001f3f), vl);
    vbool32_t is3 = __riscv_vmseq_vx_u32m1_b32(masks, UINT32_C(0x000f3f3f), vl);
    vbool32_t is4 = __riscv_vmseq_vx_u32m1_b32(masks, UINT32_C(0x073f3f3f), vl);
    decoded = __riscv_vmerge_vvm_u32m1(decoded, decoded2, is2, vl);
    decoded = __riscv_vmerge_vvm_u32m1(decoded, decoded3, is3, vl);
    decoded = __riscv_vmerge_vvm_u32m1(decoded, decoded4, is4, vl);
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
    vuint32m1_t encoded =
        __riscv_vand_vx_u32m1(values, UINT32_C(0x0000007f), vl);
    vuint32m1_t encoded2 = tokenizer_utf8_encode_2(values, vl);
    vuint32m1_t encoded3 = tokenizer_utf8_encode_3(values, vl);
    vuint32m1_t encoded4 = tokenizer_utf8_encode_4(values, vl);
    vbool32_t is2 = __riscv_vmseq_vx_u32m1_b32(masks, UINT32_C(0x00001f3f), vl);
    vbool32_t is3 = __riscv_vmseq_vx_u32m1_b32(masks, UINT32_C(0x000f3f3f), vl);
    vbool32_t is4 = __riscv_vmseq_vx_u32m1_b32(masks, UINT32_C(0x073f3f3f), vl);
    encoded = __riscv_vmerge_vvm_u32m1(encoded, encoded2, is2, vl);
    encoded = __riscv_vmerge_vvm_u32m1(encoded, encoded3, is3, vl);
    encoded = __riscv_vmerge_vvm_u32m1(encoded, encoded4, is4, vl);
    encoded = __riscv_vor_vv_u32m1(encoded, prefix_values, vl);
    __riscv_vse32_v_u32m1(packed, encoded, vl);
    codepoints += vl;
    payload_masks += vl;
    prefixes += vl;
    packed += vl;
    count -= vl;
  }
}
