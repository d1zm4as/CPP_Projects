#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace crypto {

constexpr size_t kSaltSize = 16;
constexpr size_t kIvSize = 12;
constexpr size_t kTagSize = 16;
constexpr size_t kKeySize = 32; // AES-256

bool derive_key_pbkdf2(const std::string& password,
                       const std::vector<uint8_t>& salt,
                       std::vector<uint8_t>& key_out);

bool aes_gcm_encrypt(const std::vector<uint8_t>& plaintext,
                     const std::vector<uint8_t>& key,
                     const std::vector<uint8_t>& iv,
                     std::vector<uint8_t>& ciphertext_out,
                     std::vector<uint8_t>& tag_out);

bool aes_gcm_decrypt(const std::vector<uint8_t>& ciphertext,
                     const std::vector<uint8_t>& key,
                     const std::vector<uint8_t>& iv,
                     const std::vector<uint8_t>& tag,
                     std::vector<uint8_t>& plaintext_out);

bool random_bytes(std::vector<uint8_t>& buf);

} // namespace crypto
