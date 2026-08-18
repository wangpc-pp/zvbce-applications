#include "batch.hpp"
#include "bit_permutations.hpp"

#include <array>
#include <bit>
#include <cinttypes>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>

namespace {

using p3104r0::bit_compressl;
using p3104r0::bit_compressr;
using p3104r0::bit_expandl;
using p3104r0::bit_expandr;
using p3104r0::bit_repeat;
using p3104r0::bit_reverse;
using p3104r0::next_bit_permutation;
using p3104r0::prev_bit_permutation;

template <class T>
concept has_bit_reverse = requires(T value) { bit_reverse(value); };

template <class T, class U>
concept has_bit_compressr =
    requires(T value, U mask) { bit_compressr(value, mask); };

constexpr std::size_t application_count = 1024;

alignas(64) std::array<std::uint32_t, application_count> morton_x{};
alignas(64) std::array<std::uint32_t, application_count> morton_y{};
alignas(64) std::array<std::uint32_t, application_count> morton_codes{};
alignas(64) std::array<std::uint32_t, application_count> morton_decoded_x{};
alignas(64) std::array<std::uint32_t, application_count> morton_decoded_y{};
alignas(64) std::array<std::uint32_t, application_count> morton_expanded_x{};
alignas(64) std::array<std::uint32_t, application_count> morton_expanded_y{};

alignas(64) std::array<std::uint32_t, application_count> utf8_codepoints{};
alignas(64) std::array<std::uint32_t, application_count> utf8_masks{};
alignas(64) std::array<std::uint32_t, application_count> utf8_prefixes{};
alignas(64) std::array<std::uint32_t, application_count> utf8_packed{};
alignas(64) std::array<std::uint32_t, application_count> utf8_decoded{};
alignas(64) std::array<std::uint32_t, application_count> batch_values{};
alignas(64) std::array<std::uint32_t, application_count> batch_masks{};
alignas(64) std::array<std::uint32_t, application_count> batch_actual{};
alignas(64) std::array<std::uint32_t, application_count> batch_expected{};

static_assert(bit_reverse(std::uint32_t{0x00001234}) ==
              std::uint32_t{0x2c480000});
static_assert(bit_reverse(bit_reverse(std::uint16_t{0x1234})) ==
              std::uint16_t{0x1234});
static_assert(bit_repeat(std::uint32_t{0xc}, 4) ==
              std::uint32_t{0xcccccccc});
static_assert(bit_repeat(std::uint8_t{0xa5}, 8) == std::uint8_t{0xa5});
static_assert(bit_repeat(std::uint8_t{0xa5}, 9) == std::uint8_t{0xa5});
static_assert(next_bit_permutation(std::uint8_t{0b00000111}) ==
              std::uint8_t{0b00001011});
static_assert(prev_bit_permutation(std::uint8_t{0b00001011}) ==
              std::uint8_t{0b00000111});
static_assert(bit_compressr(std::uint8_t{0b10101010},
                            std::uint8_t{0b10101010}) ==
              std::uint8_t{0b00001111});
static_assert(bit_expandr(std::uint8_t{0b00001111},
                          std::uint8_t{0b10101010}) ==
              std::uint8_t{0b10101010});
static_assert(bit_compressl(std::uint8_t{0b10101010},
                            std::uint8_t{0b10101010}) ==
              std::uint8_t{0b11110000});
static_assert(bit_expandl(std::uint8_t{0b11110000},
                          std::uint8_t{0b10101010}) ==
              std::uint8_t{0b10101010});
static_assert(noexcept(bit_reverse(std::uint32_t{})));
static_assert(!noexcept(bit_repeat(std::uint32_t{}, 1)));
static_assert(noexcept(next_bit_permutation(std::uint32_t{})));
static_assert(noexcept(prev_bit_permutation(std::uint32_t{})));
static_assert(noexcept(bit_compressr(std::uint32_t{}, std::uint32_t{})));
static_assert(noexcept(bit_expandr(std::uint32_t{}, std::uint32_t{})));
static_assert(noexcept(bit_compressl(std::uint32_t{}, std::uint32_t{})));
static_assert(noexcept(bit_expandl(std::uint32_t{}, std::uint32_t{})));
static_assert(!has_bit_reverse<bool>);
static_assert(!has_bit_reverse<int>);
static_assert(!has_bit_compressr<std::uint32_t, std::uint64_t>);
static_assert(
    std::same_as<decltype(bit_reverse(std::uint8_t{})), std::uint8_t>);
static_assert(
    std::same_as<decltype(bit_reverse(std::uint16_t{})), std::uint16_t>);
static_assert(
    std::same_as<decltype(bit_reverse(std::uint32_t{})), std::uint32_t>);
static_assert(
    std::same_as<decltype(bit_reverse(std::uint64_t{})), std::uint64_t>);

[[nodiscard]] std::uint32_t next_random(std::uint32_t &state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}

template <typename T>
[[nodiscard]] T reverse_reference(T value) {
  T result = 0;
  for (int bit = 0; bit != std::numeric_limits<T>::digits; ++bit) {
    result = static_cast<T>(result << 1);
    result = static_cast<T>(result | (value & T{1}));
    value = static_cast<T>(value >> 1);
  }
  return result;
}

template <typename T>
[[nodiscard]] T compressr_reference(T value, T mask) {
  T result = 0;
  int output_bit = 0;
  for (int input_bit = 0; input_bit != std::numeric_limits<T>::digits;
       ++input_bit) {
    if (((mask >> input_bit) & T{1}) == 0)
      continue;
    result = static_cast<T>(
        result | static_cast<T>(((value >> input_bit) & T{1}) << output_bit));
    ++output_bit;
  }
  return result;
}

template <typename T>
[[nodiscard]] T expandr_reference(T value, T mask) {
  T result = 0;
  int input_bit = 0;
  for (int output_bit = 0; output_bit != std::numeric_limits<T>::digits;
       ++output_bit) {
    if (((mask >> output_bit) & T{1}) == 0)
      continue;
    result = static_cast<T>(
        result | static_cast<T>(((value >> input_bit) & T{1}) << output_bit));
    ++input_bit;
  }
  return result;
}

template <typename T>
[[nodiscard]] T compressl_reference(T value, T mask) {
  T result = 0;
  int output_bit = std::numeric_limits<T>::digits - 1;
  for (int input_bit = std::numeric_limits<T>::digits - 1; input_bit >= 0;
       --input_bit) {
    if (((mask >> input_bit) & T{1}) == 0)
      continue;
    result = static_cast<T>(
        result | static_cast<T>(((value >> input_bit) & T{1}) << output_bit));
    --output_bit;
  }
  return result;
}

template <typename T>
[[nodiscard]] T expandl_reference(T value, T mask) {
  T result = 0;
  int input_bit = std::numeric_limits<T>::digits - 1;
  for (int output_bit = std::numeric_limits<T>::digits - 1; output_bit >= 0;
       --output_bit) {
    if (((mask >> output_bit) & T{1}) == 0)
      continue;
    result = static_cast<T>(
        result | static_cast<T>(((value >> input_bit) & T{1}) << output_bit));
    --input_bit;
  }
  return result;
}

[[nodiscard]] int popcount8(std::uint8_t value) {
  int result = 0;
  while (value != 0) {
    value = static_cast<std::uint8_t>(value & (value - 1));
    ++result;
  }
  return result;
}

[[nodiscard]] std::uint8_t next_reference(std::uint8_t value) {
  const int population = popcount8(value);
  for (unsigned candidate = static_cast<unsigned>(value) + 1; candidate < 256;
       ++candidate) {
    if (popcount8(static_cast<std::uint8_t>(candidate)) == population)
      return static_cast<std::uint8_t>(candidate);
  }
  return 0;
}

[[nodiscard]] std::uint8_t prev_reference(std::uint8_t value) {
  const int population = popcount8(value);
  for (unsigned candidate = value; candidate != 0; --candidate) {
    const auto previous = static_cast<std::uint8_t>(candidate - 1);
    if (popcount8(previous) == population)
      return previous;
  }
  return 0;
}

template <typename T>
[[nodiscard]] T repeat_reference(T value, int length) {
  T result = 0;
  for (int bit = 0; bit != std::numeric_limits<T>::digits; ++bit) {
    const int input_bit = bit % length;
    result = static_cast<T>(
        result | static_cast<T>(((value >> input_bit) & T{1}) << bit));
  }
  return result;
}

[[nodiscard]] bool check_value(const char *label, std::size_t index,
                               std::uint32_t actual,
                               std::uint32_t expected) {
  if (actual == expected)
    return true;
  std::fprintf(stderr,
               "%s failed at index %zu: actual=%08" PRIx32
               " expected=%08" PRIx32 "\n",
               label, index, actual, expected);
  return false;
}

[[nodiscard]] std::uint64_t checksum(const std::uint32_t *values,
                                     std::size_t count) {
  std::uint64_t hash = UINT64_C(1469598103934665603);
  for (std::size_t index = 0; index != count; ++index) {
    hash ^= values[index];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

[[nodiscard]] bool test_complete_uint8_domain() {
  for (unsigned value = 0; value != 256; ++value) {
    const auto x = static_cast<std::uint8_t>(value);
    if (!check_value("bit_reverse", value, bit_reverse(x),
                     reverse_reference(x)) ||
        !check_value("next_bit_permutation", value,
                     next_bit_permutation(x), next_reference(x)) ||
        !check_value("prev_bit_permutation", value,
                     prev_bit_permutation(x), prev_reference(x)))
      return false;

    for (int length = 1; length <= 8; ++length) {
      if (!check_value("bit_repeat", value, bit_repeat(x, length),
                       repeat_reference(x, length)))
        return false;
    }
    if (!check_value("bit_repeat wide length", value, bit_repeat(x, 9),
                     repeat_reference(x, 9)))
      return false;

    for (unsigned mask_value = 0; mask_value != 256; ++mask_value) {
      const auto mask = static_cast<std::uint8_t>(mask_value);
      if (!check_value("bit_compressr", value, bit_compressr(x, mask),
                       compressr_reference(x, mask)) ||
          !check_value("bit_expandr", value, bit_expandr(x, mask),
                       expandr_reference(x, mask)) ||
          !check_value("bit_compressl", value, bit_compressl(x, mask),
                       compressl_reference(x, mask)) ||
          !check_value("bit_expandl", value, bit_expandl(x, mask),
                       expandl_reference(x, mask)))
        return false;
    }
  }
  return true;
}

template <typename T>
[[nodiscard]] bool test_wide_type(const char *type_name,
                                  std::uint32_t &random_state) {
  constexpr int digits = std::numeric_limits<T>::digits;
  for (std::size_t index = 0; index != 4096; ++index) {
    T value = static_cast<T>(next_random(random_state));
    T mask = static_cast<T>(next_random(random_state));
    if constexpr (digits > 32) {
      value = static_cast<T>(value << 32) | next_random(random_state);
      mask = static_cast<T>(mask << 32) | next_random(random_state);
    }

    const int length = 1 + static_cast<int>(index % (digits + 1));
    const T next = next_bit_permutation(value);
    const T previous = prev_bit_permutation(value);

    const bool valid =
        bit_reverse(value) == reverse_reference(value) &&
        bit_repeat(value, length) == repeat_reference(value, length) &&
        bit_compressr(value, mask) == compressr_reference(value, mask) &&
        bit_expandr(value, mask) == expandr_reference(value, mask) &&
        bit_compressl(value, mask) == compressl_reference(value, mask) &&
        bit_expandl(value, mask) == expandl_reference(value, mask) &&
        (next == 0 ||
         (next > value && std::popcount(next) == std::popcount(value) &&
          prev_bit_permutation(next) == value)) &&
        (previous == 0 ||
         (previous < value &&
          std::popcount(previous) == std::popcount(value) &&
          next_bit_permutation(previous) == value));
    if (!valid) {
      std::fprintf(stderr, "wide %s test failed at index %zu\n", type_name,
                   index);
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool test_wide_types() {
  std::uint32_t random_state = 0x4f1bbcdc;
  return test_wide_type<std::uint16_t>("uint16_t", random_state) &&
         test_wide_type<std::uint32_t>("uint32_t", random_state) &&
         test_wide_type<std::uint64_t>("uint64_t", random_state);
}

[[nodiscard]] bool test_next_prev_boundaries() {
  constexpr std::array<std::uint64_t, 12> values = {
      0,
      1,
      2,
      3,
      7,
      UINT64_C(1) << 63,
      (UINT64_C(1) << 63) | 1,
      UINT64_MAX,
      UINT64_MAX - 1,
      UINT64_C(0x00000000ffffffff),
      UINT64_C(0x7fffffffffffffff),
      UINT64_C(0xf000000000000000),
  };
  constexpr std::array<std::uint64_t, values.size()> expected_next = {
      0,
      2,
      4,
      5,
      11,
      0,
      (UINT64_C(1) << 63) | 2,
      0,
      0,
      UINT64_C(0x000000017fffffff),
      UINT64_C(0xbfffffffffffffff),
      0,
  };
  constexpr std::array<std::uint64_t, values.size()> expected_previous = {
      0,
      0,
      1,
      0,
      0,
      UINT64_C(1) << 62,
      UINT64_C(0x6000000000000000),
      0,
      UINT64_C(0xfffffffffffffffd),
      0,
      0,
      UINT64_C(0xe800000000000000),
  };

  for (std::size_t index = 0; index != values.size(); ++index) {
    if (next_bit_permutation(values[index]) != expected_next[index] ||
        prev_bit_permutation(values[index]) != expected_previous[index]) {
      std::fprintf(stderr, "next/previous boundary failed at index %zu\n",
                   index);
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool test_batch_backend() {
  constexpr std::array<std::size_t, 9> counts = {0, 1, 2, 3, 7,
                                                 31, 127, 129, 1024};
  constexpr std::uint32_t shared_mask = 0x0f0f0f0f;
  std::uint32_t random_state = 0x64c2a917;
  for (std::size_t index = 0; index != application_count; ++index) {
    batch_values[index] = next_random(random_state);
    batch_masks[index] = next_random(random_state);
  }

  for (const std::size_t count : counts) {
    for (std::size_t index = 0; index != count; ++index)
      batch_expected[index] = bit_compressr(batch_values[index], shared_mask);
    p3104_batch::bit_compressr(batch_values.data(), shared_mask,
                               batch_actual.data(), count);
    for (std::size_t index = 0; index != count; ++index) {
      if (!check_value("batch compress .vx", index, batch_actual[index],
                       batch_expected[index]))
        return false;
    }

    for (std::size_t index = 0; index != count; ++index)
      batch_expected[index] = bit_expandr(batch_values[index], shared_mask);
    p3104_batch::bit_expandr(batch_values.data(), shared_mask,
                             batch_actual.data(), count);
    for (std::size_t index = 0; index != count; ++index) {
      if (!check_value("batch expand .vx", index, batch_actual[index],
                       batch_expected[index]))
        return false;
    }

    for (std::size_t index = 0; index != count; ++index)
      batch_expected[index] =
          bit_compressr(batch_values[index], batch_masks[index]);
    p3104_batch::bit_compressr(batch_values.data(), batch_masks.data(),
                               batch_actual.data(), count);
    for (std::size_t index = 0; index != count; ++index) {
      if (!check_value("batch compress .vv", index, batch_actual[index],
                       batch_expected[index]))
        return false;
    }

    for (std::size_t index = 0; index != count; ++index)
      batch_expected[index] =
          bit_expandr(batch_values[index], batch_masks[index]);
    p3104_batch::bit_expandr(batch_values.data(), batch_masks.data(),
                             batch_actual.data(), count);
    for (std::size_t index = 0; index != count; ++index) {
      if (!check_value("batch expand .vv", index, batch_actual[index],
                       batch_expected[index]))
        return false;
    }
  }

  batch_actual = batch_values;
  p3104_batch::bit_compressr(batch_actual.data(), shared_mask,
                             batch_actual.data(), batch_actual.size());
  for (std::size_t index = 0; index != batch_actual.size(); ++index) {
    if (!check_value("in-place batch compress", index, batch_actual[index],
                     bit_compressr(batch_values[index], shared_mask)))
      return false;
  }

  batch_actual = batch_values;
  p3104_batch::bit_expandr(batch_actual.data(), shared_mask,
                           batch_actual.data(), batch_actual.size());
  for (std::size_t index = 0; index != batch_actual.size(); ++index) {
    if (!check_value("in-place batch expand", index, batch_actual[index],
                     bit_expandr(batch_values[index], shared_mask)))
      return false;
  }

  batch_actual = batch_values;
  p3104_batch::bit_compressr(batch_actual.data(), batch_masks.data(),
                             batch_actual.data(), batch_actual.size());
  for (std::size_t index = 0; index != batch_actual.size(); ++index) {
    if (!check_value("in-place batch compress .vv", index,
                     batch_actual[index],
                     bit_compressr(batch_values[index], batch_masks[index])))
      return false;
  }

  batch_actual = batch_values;
  p3104_batch::bit_expandr(batch_actual.data(), batch_masks.data(),
                           batch_actual.data(), batch_actual.size());
  for (std::size_t index = 0; index != batch_actual.size(); ++index) {
    if (!check_value("in-place batch expand .vv", index, batch_actual[index],
                     bit_expandr(batch_values[index], batch_masks[index])))
      return false;
  }
  return true;
}

#if defined(__GNUC__) || defined(__clang__)
#define P3104_NOINLINE [[gnu::noinline]]
#else
#define P3104_NOINLINE
#endif

P3104_NOINLINE void morton_encode(const std::uint32_t *x,
                                  const std::uint32_t *y,
                                  std::uint32_t *codes, std::size_t count) {
  constexpr std::uint32_t even_bits = 0x55555555;
  constexpr std::uint32_t odd_bits = 0xaaaaaaaa;
  p3104_batch::bit_expandr(x, even_bits, morton_expanded_x.data(), count);
  p3104_batch::bit_expandr(y, odd_bits, morton_expanded_y.data(), count);
  for (std::size_t index = 0; index != count; ++index)
    codes[index] = morton_expanded_x[index] | morton_expanded_y[index];
}

P3104_NOINLINE void morton_decode(const std::uint32_t *codes,
                                  std::uint32_t *x, std::uint32_t *y,
                                  std::size_t count) {
  constexpr std::uint32_t even_bits = 0x55555555;
  constexpr std::uint32_t odd_bits = 0xaaaaaaaa;
  p3104_batch::bit_compressr(codes, even_bits, x, count);
  p3104_batch::bit_compressr(codes, odd_bits, y, count);
}

P3104_NOINLINE void utf8_encode_payloads(
    const std::uint32_t *codepoints, const std::uint32_t *masks,
    const std::uint32_t *prefixes, std::uint32_t *packed, std::size_t count) {
  p3104_batch::bit_expandr(codepoints, masks, packed, count);
  for (std::size_t index = 0; index != count; ++index)
    packed[index] |= prefixes[index];
}

P3104_NOINLINE void utf8_decode_payloads(const std::uint32_t *packed,
                                         const std::uint32_t *masks,
                                         std::uint32_t *codepoints,
                                         std::size_t count) {
  p3104_batch::bit_compressr(packed, masks, codepoints, count);
}

#undef P3104_NOINLINE

[[nodiscard]] bool run_bitmap_mirroring_example() {
  constexpr std::array<std::uint8_t, 8> glyph = {
      0b01110000, 0b01001000, 0b01000100, 0b01000010,
      0b01000100, 0b01001000, 0b01110000, 0b00000000};
  std::array<std::uint8_t, glyph.size()> mirrored{};

  for (std::size_t row = 0; row != glyph.size(); ++row) {
    mirrored[row] = bit_reverse(glyph[row]);
    if (!check_value("bitmap mirror", row, bit_reverse(mirrored[row]),
                     glyph[row]))
      return false;
  }

  std::printf("  1-bpp bitmap mirror: PASS, first row=0x%02x\n",
              static_cast<unsigned>(mirrored.front()));
  return true;
}

void initialize_application_inputs() {
  std::uint32_t random_state = 0x12345678;
  for (std::size_t index = 0; index != application_count; ++index) {
    const std::uint32_t random0 = next_random(random_state);
    const std::uint32_t random1 = next_random(random_state);
    morton_x[index] = random0 & 0x0000ffff;
    morton_y[index] = random1 & 0x0000ffff;

    switch (index % 4) {
    case 0:
      utf8_codepoints[index] = random0 & 0x0000007f;
      utf8_masks[index] = 0x0000007f;
      utf8_prefixes[index] = 0x00000000;
      break;
    case 1:
      utf8_codepoints[index] = 0x00000080 + random0 % 0x00000780;
      utf8_masks[index] = 0x00001f3f;
      utf8_prefixes[index] = 0x0000c080;
      break;
    case 2: {
      std::uint32_t codepoint = 0x00000800 + random0 % 0x0000f800;
      if (codepoint >= 0x0000d800 && codepoint <= 0x0000dfff)
        codepoint += 0x00000800;
      utf8_codepoints[index] = codepoint;
      utf8_masks[index] = 0x000f3f3f;
      utf8_prefixes[index] = 0x00e08080;
      break;
    }
    default:
      utf8_codepoints[index] = 0x00010000 + random0 % 0x00100000;
      utf8_masks[index] = 0x073f3f3f;
      utf8_prefixes[index] = 0xf0808080;
      break;
    }
  }

  utf8_codepoints[0] = 0x00000041;
  utf8_codepoints[1] = 0x000000a2;
  utf8_codepoints[2] = 0x00004f60;
  utf8_codepoints[3] = 0x0001f600;
}

[[nodiscard]] bool run_morton_example() {
  constexpr std::uint32_t even_bits = bit_repeat(std::uint32_t{0b01}, 2);
  constexpr std::uint32_t odd_bits = bit_repeat(std::uint32_t{0b10}, 2);
  static_assert(even_bits == 0x55555555);
  static_assert(odd_bits == 0xaaaaaaaa);

  morton_encode(morton_x.data(), morton_y.data(), morton_codes.data(),
                application_count);
  morton_decode(morton_codes.data(), morton_decoded_x.data(),
                morton_decoded_y.data(), application_count);

  for (std::size_t index = 0; index != application_count; ++index) {
    const std::uint32_t expected =
        expandr_reference(static_cast<std::uint32_t>(morton_x[index]),
                          even_bits) |
        expandr_reference(static_cast<std::uint32_t>(morton_y[index]),
                          odd_bits);
    if (!check_value("Morton encode", index, morton_codes[index], expected) ||
        !check_value("Morton decode x", index, morton_decoded_x[index],
                     morton_x[index]) ||
        !check_value("Morton decode y", index, morton_decoded_y[index],
                     morton_y[index]))
      return false;
  }

  std::printf("  Morton/Z-order:       PASS, checksum=%016" PRIx64 "\n",
              checksum(morton_codes.data(), morton_codes.size()));
  return true;
}

[[nodiscard]] bool run_utf8_example() {
  utf8_encode_payloads(utf8_codepoints.data(), utf8_masks.data(),
                       utf8_prefixes.data(), utf8_packed.data(),
                       application_count);
  utf8_decode_payloads(utf8_packed.data(), utf8_masks.data(),
                       utf8_decoded.data(), application_count);

  for (std::size_t index = 0; index != application_count; ++index) {
    const std::uint32_t expected =
        expandr_reference(utf8_codepoints[index], utf8_masks[index]) |
        utf8_prefixes[index];
    if (!check_value("UTF-8 encode", index, utf8_packed[index], expected) ||
        !check_value("UTF-8 decode", index, utf8_decoded[index],
                     utf8_codepoints[index]))
      return false;
  }

  constexpr std::array<std::uint32_t, 4> fixed_utf8 = {
      0x00000041, 0x0000c2a2, 0x00e4bda0, 0xf09f9880};
  for (std::size_t index = 0; index != fixed_utf8.size(); ++index) {
    if (!check_value("fixed UTF-8", index, utf8_packed[index],
                     fixed_utf8[index]))
      return false;
  }

  std::printf("  UTF-8 payloads:       PASS, checksum=%016" PRIx64 "\n",
              checksum(utf8_packed.data(), utf8_packed.size()));
  return true;
}

[[nodiscard]] bool run_fixed_subset_example() {
  constexpr std::array<std::uint8_t, 6> expected = {
      0b0011, 0b0101, 0b0110, 0b1001, 0b1010, 0b1100};

  std::array<std::uint8_t, expected.size()> forward{};
  std::size_t count = 0;
  for (std::uint8_t bits = 0b0011; bits < 0b10000;
       bits = next_bit_permutation(bits)) {
    if (count == forward.size())
      return false;
    forward[count++] = bits;
  }
  if (count != expected.size() || forward != expected)
    return false;

  std::array<std::uint8_t, expected.size()> reverse{};
  count = 0;
  for (std::uint8_t bits = 0b1100; bits != 0;
       bits = prev_bit_permutation(bits)) {
    if (count == reverse.size())
      return false;
    reverse[count++] = bits;
  }
  for (std::size_t index = 0; index != expected.size(); ++index) {
    if (reverse[index] != expected[expected.size() - index - 1])
      return false;
  }

  std::puts("  Fixed-size subsets:   PASS, C(4,2)=6");
  return true;
}

} // namespace

int main() {
  std::printf("Running P3104R0 tests with %s batch backend\n",
              p3104_batch::backend_name());

  if (!test_complete_uint8_domain())
    return 1;
  std::puts("  Complete uint8 tests: PASS");
  if (!test_wide_types())
    return 1;
  std::puts("  Random wide tests:    PASS");
  if (!test_next_prev_boundaries())
    return 1;
  std::puts("  Next/previous edges:  PASS");
  if (!test_batch_backend())
    return 1;
  std::puts("  Batched backend:      PASS");

  initialize_application_inputs();
  if (!run_bitmap_mirroring_example() || !run_morton_example() ||
      !run_utf8_example() || !run_fixed_subset_example())
    return 1;

  std::puts("All P3104R0 tests and applications passed");
  return 0;
}
