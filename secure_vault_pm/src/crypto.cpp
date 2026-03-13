#include "crypto.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace crypto {

bool random_bytes(std::vector<uint8_t>& buf) {
  if (buf.empty()) return true;
  return RAND_bytes(buf.data(), static_cast<int>(buf.size())) == 1;
}

bool derive_key_pbkdf2(const std::string& password,
                       const std::vector<uint8_t>& salt,
                       std::vector<uint8_t>& key_out) {
  if (salt.size() != kSaltSize) return false;
  key_out.resize(kKeySize);
  const int iterations = 100000;
  return PKCS5_PBKDF2_HMAC(
             password.c_str(),
             static_cast<int>(password.size()),
             salt.data(),
             static_cast<int>(salt.size()),
             iterations,
             EVP_sha256(),
             static_cast<int>(key_out.size()),
             key_out.data()) == 1;
}

bool aes_gcm_encrypt(const std::vector<uint8_t>& plaintext,
                     const std::vector<uint8_t>& key,
                     const std::vector<uint8_t>& iv,
                     std::vector<uint8_t>& ciphertext_out,
                     std::vector<uint8_t>& tag_out) {
  if (key.size() != kKeySize || iv.size() != kIvSize) return false;

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;

  bool ok = true;
  int len = 0;
  int ciphertext_len = 0;

  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) ok = false;
  if (ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) != 1) ok = false;
  if (ok && EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) ok = false;

  ciphertext_out.resize(plaintext.size());
  if (ok && !plaintext.empty()) {
    if (EVP_EncryptUpdate(ctx,
                          ciphertext_out.data(),
                          &len,
                          plaintext.data(),
                          static_cast<int>(plaintext.size())) != 1) ok = false;
    ciphertext_len = len;
  }

  if (ok && EVP_EncryptFinal_ex(ctx, ciphertext_out.data() + ciphertext_len, &len) != 1) ok = false;
  ciphertext_len += len;
  ciphertext_out.resize(ciphertext_len);

  tag_out.resize(kTagSize);
  if (ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(tag_out.size()), tag_out.data()) != 1) ok = false;

  EVP_CIPHER_CTX_free(ctx);
  return ok;
}

bool aes_gcm_decrypt(const std::vector<uint8_t>& ciphertext,
                     const std::vector<uint8_t>& key,
                     const std::vector<uint8_t>& iv,
                     const std::vector<uint8_t>& tag,
                     std::vector<uint8_t>& plaintext_out) {
  if (key.size() != kKeySize || iv.size() != kIvSize || tag.size() != kTagSize) return false;

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return false;

  bool ok = true;
  int len = 0;
  int plaintext_len = 0;

  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1) ok = false;
  if (ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr) != 1) ok = false;
  if (ok && EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()) != 1) ok = false;

  plaintext_out.resize(ciphertext.size());
  if (ok && !ciphertext.empty()) {
    if (EVP_DecryptUpdate(ctx,
                          plaintext_out.data(),
                          &len,
                          ciphertext.data(),
                          static_cast<int>(ciphertext.size())) != 1) ok = false;
    plaintext_len = len;
  }

  if (ok && EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), const_cast<uint8_t*>(tag.data())) != 1) ok = false;
  if (ok && EVP_DecryptFinal_ex(ctx, plaintext_out.data() + plaintext_len, &len) != 1) ok = false;
  plaintext_len += len;
  plaintext_out.resize(plaintext_len);

  EVP_CIPHER_CTX_free(ctx);
  return ok;
}

} // namespace crypto
