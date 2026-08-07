#ifndef ZVBEXT_APPLICATIONS_H
#define ZVBEXT_APPLICATIONS_H

#include <stddef.h>
#include <stdint.h>

const char *implementation_name(void);

void base64_decode_reshape(const uint32_t *input, uint32_t *output,
                           size_t count);
void base64_encode_reshape(const uint32_t *input, uint32_t *output,
                           size_t count);
void uleb28_encode_payloads(const uint32_t *input, uint32_t *output,
                            size_t count);
void uleb28_decode_payloads(const uint32_t *input, uint32_t *output,
                            size_t count);
void morton2d_encode(const uint32_t *x, const uint32_t *y, uint32_t *output,
                     size_t count);
void morton2d_decode(const uint32_t *input, uint32_t *x, uint32_t *y,
                     size_t count);
void tokenizer_utf8_decode(const uint32_t *packed,
                           const uint32_t *payload_masks, uint32_t *codepoints,
                           size_t count);
void tokenizer_utf8_encode(const uint32_t *codepoints,
                           const uint32_t *payload_masks,
                           const uint32_t *prefixes, uint32_t *packed,
                           size_t count);

#endif
