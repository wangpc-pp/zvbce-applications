#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "applications.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#define TEST_COUNT 257

static uint32_t base64_words[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t base64_payloads[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t base64_roundtrip[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t uleb_values[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t uleb_slots[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t uleb_roundtrip[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t morton_x[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t morton_y[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t morton_codes[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t morton_decoded_x[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t morton_decoded_y[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t tokenizer_codepoints[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t tokenizer_payload_masks[TEST_COUNT]
    __attribute__((aligned(64)));
static uint32_t tokenizer_prefixes[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t tokenizer_packed[TEST_COUNT] __attribute__((aligned(64)));
static uint32_t tokenizer_decoded[TEST_COUNT] __attribute__((aligned(64)));

static uint32_t pext32_reference(uint32_t value, uint32_t mask) {
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

static uint32_t pdep32_reference(uint32_t value, uint32_t mask) {
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

static uint32_t next_random(uint32_t *state) {
  uint32_t value = *state;
  value ^= value << 13;
  value ^= value >> 17;
  value ^= value << 5;
  *state = value;
  return value;
}

static int check_value(const char *test_name, size_t index, uint32_t actual,
                       uint32_t expected) {
  if (actual == expected)
    return 0;

  fprintf(stderr,
          "%s failed at index %zu: expected 0x%08" PRIx32 ", got 0x%08" PRIx32
          "\n",
          test_name, index, expected, actual);
  return 1;
}

static uint64_t checksum(const uint32_t *values, size_t count) {
  uint64_t hash = UINT64_C(0xcbf29ce484222325);

  for (size_t index = 0; index != count; ++index) {
    hash ^= values[index];
    hash *= UINT64_C(0x100000001b3);
  }
  return hash;
}

static int run_base64_tests(void) {
  const uint32_t mask = UINT32_C(0x3f3f3f3f);

  base64_decode_reshape(base64_words, base64_payloads,
                        ARRAY_SIZE(base64_words));
  base64_encode_reshape(base64_payloads, base64_roundtrip,
                        ARRAY_SIZE(base64_payloads));

  for (size_t index = 0; index != ARRAY_SIZE(base64_words); ++index) {
    uint32_t expected_payload = pext32_reference(base64_words[index], mask);
    if (check_value("Base64 decode reshape", index, base64_payloads[index],
                    expected_payload))
      return 1;
    if (check_value("Base64 encode reshape", index, base64_roundtrip[index],
                    base64_words[index]))
      return 1;
  }

  printf("  Base64 reshape:       PASS, checksum=%016" PRIx64 "\n",
         checksum(base64_payloads, ARRAY_SIZE(base64_payloads)));
  return 0;
}

static int run_uleb128_tests(void) {
  const uint32_t mask = UINT32_C(0x7f7f7f7f);

  uleb28_encode_payloads(uleb_values, uleb_slots, ARRAY_SIZE(uleb_values));
  uleb28_decode_payloads(uleb_slots, uleb_roundtrip, ARRAY_SIZE(uleb_slots));

  for (size_t index = 0; index != ARRAY_SIZE(uleb_values); ++index) {
    uint32_t expected_slots = pdep32_reference(uleb_values[index], mask);
    if (check_value("ULEB128 payload spread", index, uleb_slots[index],
                    expected_slots))
      return 1;
    if (check_value("ULEB128 payload gather", index, uleb_roundtrip[index],
                    uleb_values[index]))
      return 1;
  }

  printf("  ULEB128 payloads:     PASS, checksum=%016" PRIx64 "\n",
         checksum(uleb_slots, ARRAY_SIZE(uleb_slots)));
  return 0;
}

static int run_morton_tests(void) {
  const uint32_t x_mask = UINT32_C(0x55555555);
  const uint32_t y_mask = UINT32_C(0xaaaaaaaa);

  morton2d_encode(morton_x, morton_y, morton_codes, ARRAY_SIZE(morton_x));
  morton2d_decode(morton_codes, morton_decoded_x, morton_decoded_y,
                  ARRAY_SIZE(morton_codes));

  for (size_t index = 0; index != ARRAY_SIZE(morton_x); ++index) {
    uint32_t expected_code = pdep32_reference(morton_x[index], x_mask) |
                             pdep32_reference(morton_y[index], y_mask);
    if (check_value("Morton encode", index, morton_codes[index], expected_code))
      return 1;
    if (check_value("Morton decode x", index, morton_decoded_x[index],
                    morton_x[index]))
      return 1;
    if (check_value("Morton decode y", index, morton_decoded_y[index],
                    morton_y[index]))
      return 1;
  }

  printf("  Morton 2D:            PASS, checksum=%016" PRIx64 "\n",
         checksum(morton_codes, ARRAY_SIZE(morton_codes)));
  return 0;
}

static int run_tokenizer_utf8_tests(void) {
  static const uint32_t fixed_packed[] = {
      UINT32_C(0x00000041),
      UINT32_C(0x0000c2a2),
      UINT32_C(0x00e4bda0),
      UINT32_C(0xf09f9880),
  };

  tokenizer_utf8_encode(tokenizer_codepoints, tokenizer_payload_masks,
                        tokenizer_prefixes, tokenizer_packed,
                        ARRAY_SIZE(tokenizer_codepoints));
  tokenizer_utf8_decode(tokenizer_packed, tokenizer_payload_masks,
                        tokenizer_decoded, ARRAY_SIZE(tokenizer_packed));

  for (size_t index = 0; index != ARRAY_SIZE(tokenizer_codepoints); ++index) {
    uint32_t expected_packed =
        pdep32_reference(tokenizer_codepoints[index],
                         tokenizer_payload_masks[index]) |
        tokenizer_prefixes[index];
    if (check_value("Tokenizer UTF-8 encode", index, tokenizer_packed[index],
                    expected_packed))
      return 1;
    if (check_value("Tokenizer UTF-8 decode", index, tokenizer_decoded[index],
                    tokenizer_codepoints[index]))
      return 1;
  }
  for (size_t index = 0; index != ARRAY_SIZE(fixed_packed); ++index) {
    if (check_value("Tokenizer fixed UTF-8 encode", index,
                    tokenizer_packed[index], fixed_packed[index]))
      return 1;
  }

  printf("  Tokenizer UTF-8:      PASS, checksum=%016" PRIx64 "\n",
         checksum(tokenizer_packed, ARRAY_SIZE(tokenizer_packed)));
  return 0;
}

static void initialize_inputs(void) {
  uint32_t random_state = UINT32_C(0x12345678);

  for (size_t index = 0; index != TEST_COUNT; ++index) {
    uint32_t random0 = next_random(&random_state);
    uint32_t random1 = next_random(&random_state);
    uint32_t a = random0 & 0x3f;
    uint32_t b = (random0 >> 6) & 0x3f;
    uint32_t c = (random0 >> 12) & 0x3f;
    uint32_t d = (random0 >> 18) & 0x3f;

    base64_words[index] = d | (c << 8) | (b << 16) | (a << 24);
    uleb_values[index] = random1 & UINT32_C(0x0fffffff);
    morton_x[index] = random0 & UINT32_C(0x0000ffff);
    morton_y[index] = random1 & UINT32_C(0x0000ffff);

    switch (index % 4) {
    case 0:
      tokenizer_codepoints[index] = random0 & UINT32_C(0x0000007f);
      tokenizer_payload_masks[index] = UINT32_C(0x0000007f);
      tokenizer_prefixes[index] = UINT32_C(0x00000000);
      break;
    case 1:
      tokenizer_codepoints[index] =
          UINT32_C(0x00000080) + random0 % UINT32_C(0x00000780);
      tokenizer_payload_masks[index] = UINT32_C(0x00001f3f);
      tokenizer_prefixes[index] = UINT32_C(0x0000c080);
      break;
    case 2: {
      uint32_t codepoint =
          UINT32_C(0x00000800) + random0 % UINT32_C(0x0000f800);
      if (codepoint >= UINT32_C(0x0000d800) &&
          codepoint <= UINT32_C(0x0000dfff))
        codepoint += UINT32_C(0x00000800);
      tokenizer_codepoints[index] = codepoint;
      tokenizer_payload_masks[index] = UINT32_C(0x000f3f3f);
      tokenizer_prefixes[index] = UINT32_C(0x00e08080);
      break;
    }
    default:
      tokenizer_codepoints[index] =
          UINT32_C(0x00010000) + random0 % UINT32_C(0x00100000);
      tokenizer_payload_masks[index] = UINT32_C(0x073f3f3f);
      tokenizer_prefixes[index] = UINT32_C(0xf0808080);
      break;
    }
  }

  tokenizer_codepoints[0] = UINT32_C(0x00000041);
  tokenizer_codepoints[1] = UINT32_C(0x000000a2);
  tokenizer_codepoints[2] = UINT32_C(0x00004f60);
  tokenizer_codepoints[3] = UINT32_C(0x0001f600);
}

int main(void) {
  initialize_inputs();

  printf("Running %s application tests with %u elements\n",
         implementation_name(), TEST_COUNT);
  if (run_base64_tests() || run_uleb128_tests() || run_morton_tests() ||
      run_tokenizer_utf8_tests())
    return 1;

  puts("All application tests passed");
  return 0;
}
