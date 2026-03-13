#pragma once

#include <string>
#include <vector>

namespace password_db {

struct Entry {
  std::string site;
  std::string user;
  std::string pass;
  std::string note;
};

bool load(const std::string& vault_path,
          const std::string& password,
          std::vector<Entry>& entries,
          std::string& err);

bool save(const std::string& vault_path,
          const std::string& password,
          const std::vector<Entry>& entries,
          std::string& err);

std::string serialize(const std::vector<Entry>& entries);

bool deserialize(const std::string& data,
                 std::vector<Entry>& entries,
                 std::string& err);

bool upsert(std::vector<Entry>& entries, const Entry& e);

bool remove(std::vector<Entry>& entries, const std::string& site);

const Entry* find(const std::vector<Entry>& entries, const std::string& site);

} // namespace password_db
