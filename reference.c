#include <stddef.h>
#include <stdint.h>

#include "applications.h"

const char *implementation_name(void) { return "scalar C reference"; }

static uint32_t pext32(uint32_t value, uint32_t mask) {
  uint32_t result = 0;
  uint32_t result_bit = 1;

  while (mask != 0) {
    uint32_t selected_bit = mask & -mask;
    if ((value & selected_bit) != 0)
      result |= result_bit;
    mask &= mask - 1;
    result_bit <<= 1;
  }
  return result;
}

static uint32_t pdep32(uint32_t value, uint32_t mask) {
  uint32_t result = 0;

  while (mask != 0) {
    uint32_t selected_bit = mask & -mask;
    if ((value & 1) != 0)
      result |= selected_bit;
    value >>= 1;
    mask &= mask - 1;
  }
  return result;
}

void base64_decode_reshape(const uint32_t *input, uint32_t *output,
                           size_t count) {
  for (size_t index = 0; index != count; ++index)
    output[index] = pext32(input[index], UINT32_C(0x3f3f3f3f));
}

void base64_encode_reshape(const uint32_t *input, uint32_t *output,
                           size_t count) {
  for (size_t index = 0; index != count; ++index)
    output[index] = pdep32(input[index], UINT32_C(0x3f3f3f3f));
}

void uleb28_encode_payloads(const uint32_t *input, uint32_t *output,
                            size_t count) {
  for (size_t index = 0; index != count; ++index)
    output[index] = pdep32(input[index], UINT32_C(0x7f7f7f7f));
}

void uleb28_decode_payloads(const uint32_t *input, uint32_t *output,
                            size_t count) {
  for (size_t index = 0; index != count; ++index)
    output[index] = pext32(input[index], UINT32_C(0x7f7f7f7f));
}

void morton2d_encode(const uint32_t *x, const uint32_t *y, uint32_t *output,
                     size_t count) {
  for (size_t index = 0; index != count; ++index)
    output[index] = pdep32(x[index], UINT32_C(0x55555555)) |
                    pdep32(y[index], UINT32_C(0xaaaaaaaa));
}

void morton2d_decode(const uint32_t *input, uint32_t *x, uint32_t *y,
                     size_t count) {
  for (size_t index = 0; index != count; ++index) {
    x[index] = pext32(input[index], UINT32_C(0x55555555));
    y[index] = pext32(input[index], UINT32_C(0xaaaaaaaa));
  }
}

void tokenizer_utf8_decode(const uint32_t *packed,
                           const uint32_t *payload_masks, uint32_t *codepoints,
                           size_t count) {
  for (size_t index = 0; index != count; ++index)
    codepoints[index] = pext32(packed[index], payload_masks[index]);
}

void tokenizer_utf8_encode(const uint32_t *codepoints,
                           const uint32_t *payload_masks,
                           const uint32_t *prefixes, uint32_t *packed,
                           size_t count) {
  for (size_t index = 0; index != count; ++index)
    packed[index] =
        pdep32(codepoints[index], payload_masks[index]) | prefixes[index];
}
