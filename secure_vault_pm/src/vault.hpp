#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vault {

bool encrypt_file(const std::string& in_path,
                  const std::string& out_path,
                  const std::string& password,
                  std::string& err);

bool decrypt_file(const std::string& in_path,
                  const std::string& out_path,
                  const std::string& password,
                  std::string& err);

bool encrypt_bytes_to_vault(const std::vector<uint8_t>& plaintext,
                            const std::string& password,
                            std::vector<uint8_t>& out_vault_bytes,
                            std::string& err);

bool decrypt_bytes_from_vault(const std::vector<uint8_t>& vault_bytes,
                              const std::string& password,
                              std::vector<uint8_t>& out_plaintext,
                              std::string& err);

} // namespace vault
