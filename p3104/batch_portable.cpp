#include "batch.hpp"

#include "bit_permutations.hpp"

namespace p3104_batch {

const char *backend_name() noexcept { return "portable scalar C++"; }

void bit_compressr(const std::uint32_t *input, std::uint32_t mask,
                   std::uint32_t *output, std::size_t count) {
  for (std::size_t index = 0; index != count; ++index)
    output[index] = p3104r0::bit_compressr(input[index], mask);
}

void bit_expandr(const std::uint32_t *input, std::uint32_t mask,
                 std::uint32_t *output, std::size_t count) {
  for (std::size_t index = 0; index != count; ++index)
    output[index] = p3104r0::bit_expandr(input[index], mask);
}

void bit_compressr(const std::uint32_t *input, const std::uint32_t *masks,
                   std::uint32_t *output, std::size_t count) {
  for (std::size_t index = 0; index != count; ++index)
    output[index] = p3104r0::bit_compressr(input[index], masks[index]);
}

void bit_expandr(const std::uint32_t *input, const std::uint32_t *masks,
                 std::uint32_t *output, std::size_t count) {
  for (std::size_t index = 0; index != count; ++index)
    output[index] = p3104r0::bit_expandr(input[index], masks[index]);
}

} // namespace p3104_batch
