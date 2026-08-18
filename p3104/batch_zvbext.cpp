#include "batch.hpp"

#include <riscv_vector.h>

namespace p3104_batch {

const char *backend_name() noexcept { return "RVV + Zvbext"; }

void bit_compressr(const std::uint32_t *input, std::uint32_t mask,
                   std::uint32_t *output, std::size_t count) {
  while (count != 0) {
    const std::size_t vl = __riscv_vsetvl_e32m1(count);
    const vuint32m1_t values = __riscv_vle32_v_u32m1(input, vl);
    const vuint32m1_t compressed =
        __riscv_vbcompress_vx_u32m1(values, mask, vl);
    __riscv_vse32_v_u32m1(output, compressed, vl);
    input += vl;
    output += vl;
    count -= vl;
  }
}

void bit_expandr(const std::uint32_t *input, std::uint32_t mask,
                 std::uint32_t *output, std::size_t count) {
  while (count != 0) {
    const std::size_t vl = __riscv_vsetvl_e32m1(count);
    const vuint32m1_t values = __riscv_vle32_v_u32m1(input, vl);
    const vuint32m1_t expanded = __riscv_vbexpand_vx_u32m1(values, mask, vl);
    __riscv_vse32_v_u32m1(output, expanded, vl);
    input += vl;
    output += vl;
    count -= vl;
  }
}

void bit_compressr(const std::uint32_t *input, const std::uint32_t *masks,
                   std::uint32_t *output, std::size_t count) {
  while (count != 0) {
    const std::size_t vl = __riscv_vsetvl_e32m1(count);
    const vuint32m1_t values = __riscv_vle32_v_u32m1(input, vl);
    const vuint32m1_t mask_values = __riscv_vle32_v_u32m1(masks, vl);
    const vuint32m1_t compressed =
        __riscv_vbcompress_vv_u32m1(values, mask_values, vl);
    __riscv_vse32_v_u32m1(output, compressed, vl);
    input += vl;
    masks += vl;
    output += vl;
    count -= vl;
  }
}

void bit_expandr(const std::uint32_t *input, const std::uint32_t *masks,
                 std::uint32_t *output, std::size_t count) {
  while (count != 0) {
    const std::size_t vl = __riscv_vsetvl_e32m1(count);
    const vuint32m1_t values = __riscv_vle32_v_u32m1(input, vl);
    const vuint32m1_t mask_values = __riscv_vle32_v_u32m1(masks, vl);
    const vuint32m1_t expanded =
        __riscv_vbexpand_vv_u32m1(values, mask_values, vl);
    __riscv_vse32_v_u32m1(output, expanded, vl);
    input += vl;
    masks += vl;
    output += vl;
    count -= vl;
  }
}

} // namespace p3104_batch
