#ifndef P3104_BATCH_HPP
#define P3104_BATCH_HPP

#include <cstddef>
#include <cstdint>

// Batched forms of the P3104R0 right-oriented operations.  These are not part
// of the proposal; they are the application boundary used to map independent
// scalar operations to element-wise RVV Zvbext instructions.
namespace p3104_batch {

[[nodiscard]] const char *backend_name() noexcept;

// One shared mask maps to the Zvbext .vx forms.
void bit_compressr(const std::uint32_t *input, std::uint32_t mask,
                   std::uint32_t *output, std::size_t count);
void bit_expandr(const std::uint32_t *input, std::uint32_t mask,
                 std::uint32_t *output, std::size_t count);

// One mask per element maps to the Zvbext .vv forms.
void bit_compressr(const std::uint32_t *input, const std::uint32_t *masks,
                   std::uint32_t *output, std::size_t count);
void bit_expandr(const std::uint32_t *input, const std::uint32_t *masks,
                 std::uint32_t *output, std::size_t count);

} // namespace p3104_batch

#endif
