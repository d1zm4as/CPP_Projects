#include "vault.hpp"
#include "crypto.hpp"

#include <fstream>
#include <sstream>

namespace vault {

namespace {

constexpr uint8_t kVersion = 1;
const uint8_t kMagic[4] = {'S', 'V', 'P', 'M'};

bool read_file(const std::string& path, std::vector<uint8_t>& out, std::string& err) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    err = "Failed to open input file";
    return false;
  }
  in.seekg(0, std::ios::end);
  std::streamoff size = in.tellg();
  if (size < 0) {
    err = "Failed to read file size";
    return false;
  }
  in.seekg(0, std::ios::beg);
  out.resize(static_cast<size_t>(size));
  if (!out.empty()) {
    in.read(reinterpret_cast<char*>(out.data()), size);
    if (!in) {
      err = "Failed to read file";
      return false;
    }
  }
  return true;
}

bool write_file(const std::string& path, const std::vector<uint8_t>& data, std::string& err) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    err = "Failed to open output file";
    return false;
  }
  if (!data.empty()) {
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out) {
      err = "Failed to write output file";
      return false;
    }
  }
  return true;
}

void write_u32_be(std::vector<uint8_t>& out, uint32_t v) {
  out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
  out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
  out.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
  out.push_back(static_cast<uint8_t>(v & 0xFF));
}

bool read_u32_be(const std::vector<uint8_t>& in, size_t& offset, uint32_t& v) {
  if (offset + 4 > in.size()) return false;
  v = (static_cast<uint32_t>(in[offset]) << 24) |
      (static_cast<uint32_t>(in[offset + 1]) << 16) |
      (static_cast<uint32_t>(in[offset + 2]) << 8) |
      static_cast<uint32_t>(in[offset + 3]);
  offset += 4;
  return true;
}

} // namespace

bool encrypt_bytes_to_vault(const std::vector<uint8_t>& plaintext,
                            const std::string& password,
                            std::vector<uint8_t>& out_vault_bytes,
                            std::string& err) {
  std::vector<uint8_t> salt(crypto::kSaltSize);
  std::vector<uint8_t> iv(crypto::kIvSize);
  if (!crypto::random_bytes(salt) || !crypto::random_bytes(iv)) {
    err = "Failed to generate randomness";
    return false;
  }

  std::vector<uint8_t> key;
  if (!crypto::derive_key_pbkdf2(password, salt, key)) {
    err = "Key derivation failed";
    return false;
  }

  std::vector<uint8_t> ciphertext;
  std::vector<uint8_t> tag;
  if (!crypto::aes_gcm_encrypt(plaintext, key, iv, ciphertext, tag)) {
    err = "Encryption failed";
    return false;
  }

  out_vault_bytes.clear();
  out_vault_bytes.insert(out_vault_bytes.end(), kMagic, kMagic + 4);
  out_vault_bytes.push_back(kVersion);
  out_vault_bytes.insert(out_vault_bytes.end(), salt.begin(), salt.end());
  out_vault_bytes.insert(out_vault_bytes.end(), iv.begin(), iv.end());
  out_vault_bytes.insert(out_vault_bytes.end(), tag.begin(), tag.end());
  write_u32_be(out_vault_bytes, static_cast<uint32_t>(ciphertext.size()));
  out_vault_bytes.insert(out_vault_bytes.end(), ciphertext.begin(), ciphertext.end());

  return true;
}

bool decrypt_bytes_from_vault(const std::vector<uint8_t>& vault_bytes,
                              const std::string& password,
                              std::vector<uint8_t>& out_plaintext,
                              std::string& err) {
  if (vault_bytes.size() < 4 + 1 + crypto::kSaltSize + crypto::kIvSize + crypto::kTagSize + 4) {
    err = "Vault file too small";
    return false;
  }

  size_t offset = 0;
  if (!(vault_bytes[0] == kMagic[0] && vault_bytes[1] == kMagic[1] &&
        vault_bytes[2] == kMagic[2] && vault_bytes[3] == kMagic[3])) {
    err = "Invalid vault magic";
    return false;
  }
  offset += 4;

  uint8_t version = vault_bytes[offset++];
  if (version != kVersion) {
    err = "Unsupported vault version";
    return false;
  }

  std::vector<uint8_t> salt(vault_bytes.begin() + offset, vault_bytes.begin() + offset + crypto::kSaltSize);
  offset += crypto::kSaltSize;

  std::vector<uint8_t> iv(vault_bytes.begin() + offset, vault_bytes.begin() + offset + crypto::kIvSize);
  offset += crypto::kIvSize;

  std::vector<uint8_t> tag(vault_bytes.begin() + offset, vault_bytes.begin() + offset + crypto::kTagSize);
  offset += crypto::kTagSize;

  uint32_t ciphertext_len = 0;
  if (!read_u32_be(vault_bytes, offset, ciphertext_len)) {
    err = "Invalid ciphertext length";
    return false;
  }

  if (offset + ciphertext_len != vault_bytes.size()) {
    err = "Vault file size mismatch";
    return false;
  }

  std::vector<uint8_t> ciphertext(vault_bytes.begin() + offset, vault_bytes.end());

  std::vector<uint8_t> key;
  if (!crypto::derive_key_pbkdf2(password, salt, key)) {
    err = "Key derivation failed";
    return false;
  }

  if (!crypto::aes_gcm_decrypt(ciphertext, key, iv, tag, out_plaintext)) {
    err = "Decryption failed (wrong password or corrupted file)";
    return false;
  }

  return true;
}

bool encrypt_file(const std::string& in_path,
                  const std::string& out_path,
                  const std::string& password,
                  std::string& err) {
  std::vector<uint8_t> plaintext;
  if (!read_file(in_path, plaintext, err)) return false;

  std::vector<uint8_t> vault_bytes;
  if (!encrypt_bytes_to_vault(plaintext, password, vault_bytes, err)) return false;

  return write_file(out_path, vault_bytes, err);
}

bool decrypt_file(const std::string& in_path,
                  const std::string& out_path,
                  const std::string& password,
                  std::string& err) {
  std::vector<uint8_t> vault_bytes;
  if (!read_file(in_path, vault_bytes, err)) return false;

  std::vector<uint8_t> plaintext;
  if (!decrypt_bytes_from_vault(vault_bytes, password, plaintext, err)) return false;

  return write_file(out_path, plaintext, err);
}

} // namespace vault
