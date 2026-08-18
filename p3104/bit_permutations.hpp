#ifndef P3104_BIT_PERMUTATIONS_HPP
#define P3104_BIT_PERMUTATIONS_HPP

#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

// P3104R0 proposes these functions for <bit>.  This compatibility header keeps
// them in a separate namespace because adding declarations to namespace std is
// undefined behavior for an application.
namespace p3104r0 {
namespace detail {

#if defined(__SIZEOF_INT128__)
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
using uint128_t = unsigned __int128;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

template <class T>
inline constexpr bool is_unsigned_integer =
    std::same_as<std::remove_cv_t<T>, unsigned char> ||
    std::same_as<std::remove_cv_t<T>, unsigned short> ||
    std::same_as<std::remove_cv_t<T>, unsigned int> ||
    std::same_as<std::remove_cv_t<T>, unsigned long> ||
    std::same_as<std::remove_cv_t<T>, unsigned long long>
#if defined(__SIZEOF_INT128__)
    || std::same_as<std::remove_cv_t<T>, uint128_t>
#endif
    ;

} // namespace detail

template <class T>
concept unsigned_integer = detail::is_unsigned_integer<T>;

namespace detail {

template <unsigned_integer T>
inline constexpr int digits = std::numeric_limits<T>::digits;

template <unsigned_integer T>
[[nodiscard]] constexpr T bit_reverse_fallback(T value) noexcept {
  T result = 0;
  for (int index = 0; index != digits<T>; ++index) {
    result = static_cast<T>(result << 1);
    result = static_cast<T>(result | (value & T{1}));
    value = static_cast<T>(value >> 1);
  }
  return result;
}

template <unsigned_integer T>
[[nodiscard]] constexpr T bit_compressr_fallback(T value, T mask) noexcept {
  T result = 0;
  T output_bit = 1;

  while (mask != 0) {
    const T selected_bit = static_cast<T>(mask & static_cast<T>(T{0} - mask));
    if ((value & selected_bit) != 0)
      result = static_cast<T>(result | output_bit);
    mask = static_cast<T>(mask & static_cast<T>(mask - 1));
    output_bit = static_cast<T>(output_bit << 1);
  }
  return result;
}

template <unsigned_integer T>
[[nodiscard]] constexpr T bit_expandr_fallback(T value, T mask) noexcept {
  T result = 0;
  T input_bit = 1;

  while (mask != 0) {
    const T selected_bit = static_cast<T>(mask & static_cast<T>(T{0} - mask));
    if ((value & input_bit) != 0)
      result = static_cast<T>(result | selected_bit);
    mask = static_cast<T>(mask & static_cast<T>(mask - 1));
    input_bit = static_cast<T>(input_bit << 1);
  }
  return result;
}

} // namespace detail

/// Reverses all bits in value.
template <unsigned_integer T>
[[nodiscard]] constexpr T bit_reverse(T value) noexcept {
#if defined(__has_builtin)
  if constexpr (detail::digits<T> == 8) {
#if __has_builtin(__builtin_bitreverse8)
    return static_cast<T>(__builtin_bitreverse8(static_cast<std::uint8_t>(value)));
#endif
  } else if constexpr (detail::digits<T> == 16) {
#if __has_builtin(__builtin_bitreverse16)
    return static_cast<T>(
        __builtin_bitreverse16(static_cast<std::uint16_t>(value)));
#endif
  } else if constexpr (detail::digits<T> == 32) {
#if __has_builtin(__builtin_bitreverse32)
    return static_cast<T>(
        __builtin_bitreverse32(static_cast<std::uint32_t>(value)));
#endif
  } else if constexpr (detail::digits<T> == 64) {
#if __has_builtin(__builtin_bitreverse64)
    return static_cast<T>(
        __builtin_bitreverse64(static_cast<std::uint64_t>(value)));
#endif
  }
#endif
  return detail::bit_reverse_fallback(value);
}

/// Repeats the least-significant length bits of value across the result.
/// P3104R0 specifies the precondition length > 0.
template <unsigned_integer T>
[[nodiscard]] constexpr T bit_repeat(T value, int length) {
  T result = 0;
  for (int index = 0; index != detail::digits<T>; ++index) {
    const int source_index = index % length;
    const T bit = static_cast<T>((value >> source_index) & T{1});
    result = static_cast<T>(result | static_cast<T>(bit << index));
  }
  return result;
}

/// Returns the next larger value with the same population count, or zero.
template <unsigned_integer T>
[[nodiscard]] constexpr T next_bit_permutation(T value) noexcept {
  if (value == 0)
    return 0;

  const T least_bit =
      static_cast<T>(value & static_cast<T>(T{0} - value));
  const T incremented = static_cast<T>(value + least_bit);
  if (incremented == 0)
    return 0;

  T packed_ones = static_cast<T>(value ^ incremented);
  packed_ones = static_cast<T>((packed_ones >> 2) / least_bit);
  return static_cast<T>(incremented | packed_ones);
}

/// Returns the next smaller value with the same population count, or zero.
template <unsigned_integer T>
[[nodiscard]] constexpr T prev_bit_permutation(T value) noexcept {
  const T complemented = static_cast<T>(~value);
  const T successor = next_bit_permutation(complemented);
  return successor == 0 ? T{0} : static_cast<T>(~successor);
}

/// Packs the bits selected by mask into the least-significant result bits.
template <unsigned_integer T>
[[nodiscard]] constexpr T bit_compressr(T value, T mask) noexcept {
#if defined(__has_builtin)
#if __has_builtin(__builtin_elementwise_pext)
  return __builtin_elementwise_pext(value, mask);
#endif
#endif
  return detail::bit_compressr_fallback(value, mask);
}

/// Expands the least-significant input bits into positions selected by mask.
template <unsigned_integer T>
[[nodiscard]] constexpr T bit_expandr(T value, T mask) noexcept {
#if defined(__has_builtin)
#if __has_builtin(__builtin_elementwise_pdep)
  return __builtin_elementwise_pdep(value, mask);
#endif
#endif
  return detail::bit_expandr_fallback(value, mask);
}

/// Packs selected bits against the most-significant edge of the result.
template <unsigned_integer T>
[[nodiscard]] constexpr T bit_compressl(T value, T mask) noexcept {
  return bit_reverse(
      bit_compressr(bit_reverse(value), bit_reverse(mask)));
}

/// Expands the most-significant input bits into positions selected by mask.
template <unsigned_integer T>
[[nodiscard]] constexpr T bit_expandl(T value, T mask) noexcept {
  return bit_reverse(bit_expandr(bit_reverse(value), bit_reverse(mask)));
}

} // namespace p3104r0

#endif
