#include "password_db.hpp"
#include "vault.hpp"

#include <fstream>
#include <sstream>

namespace password_db {

namespace {

std::string escape_field(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    if (c == '\\') {
      out += "\\\\";
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\t') {
      out += "\\t";
    } else {
      out += c;
    }
  }
  return out;
}

bool unescape_field(const std::string& s, std::string& out) {
  out.clear();
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    char c = s[i];
    if (c == '\\') {
      if (i + 1 >= s.size()) return false;
      char n = s[++i];
      if (n == 'n') out += '\n';
      else if (n == 't') out += '\t';
      else if (n == '\\') out += '\\';
      else return false;
    } else {
      out += c;
    }
  }
  return true;
}

} // namespace

std::string serialize(const std::vector<Entry>& entries) {
  std::ostringstream oss;
  for (const auto& e : entries) {
    oss << escape_field(e.site) << '\t'
        << escape_field(e.user) << '\t'
        << escape_field(e.pass) << '\t'
        << escape_field(e.note) << '\n';
  }
  return oss.str();
}

bool deserialize(const std::string& data,
                 std::vector<Entry>& entries,
                 std::string& err) {
  entries.clear();
  std::istringstream iss(data);
  std::string line;
  while (std::getline(iss, line)) {
    if (line.empty()) continue;
    std::vector<std::string> fields;
    size_t start = 0;
    while (true) {
      size_t pos = line.find('\t', start);
      if (pos == std::string::npos) {
        fields.push_back(line.substr(start));
        break;
      }
      fields.push_back(line.substr(start, pos - start));
      start = pos + 1;
    }
    if (fields.size() != 4) {
      err = "Invalid entry format";
      return false;
    }
    Entry e;
    if (!unescape_field(fields[0], e.site) ||
        !unescape_field(fields[1], e.user) ||
        !unescape_field(fields[2], e.pass) ||
        !unescape_field(fields[3], e.note)) {
      err = "Invalid escape sequence";
      return false;
    }
    entries.push_back(std::move(e));
  }
  return true;
}

bool load(const std::string& vault_path,
          const std::string& password,
          std::vector<Entry>& entries,
          std::string& err) {
  std::ifstream in(vault_path, std::ios::binary);
  if (!in) {
    err = "Failed to open vault file";
    return false;
  }
  std::vector<uint8_t> vault_bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (vault_bytes.empty()) {
    err = "Vault file is empty";
    return false;
  }

  std::vector<uint8_t> plaintext;
  if (!vault::decrypt_bytes_from_vault(vault_bytes, password, plaintext, err)) return false;

  std::string data(plaintext.begin(), plaintext.end());
  return deserialize(data, entries, err);
}

bool save(const std::string& vault_path,
          const std::string& password,
          const std::vector<Entry>& entries,
          std::string& err) {
  std::string data = serialize(entries);
  std::vector<uint8_t> plaintext(data.begin(), data.end());
  std::vector<uint8_t> vault_bytes;
  if (!vault::encrypt_bytes_to_vault(plaintext, password, vault_bytes, err)) return false;

  std::ofstream out(vault_path, std::ios::binary);
  if (!out) {
    err = "Failed to write vault file";
    return false;
  }
  out.write(reinterpret_cast<const char*>(vault_bytes.data()), static_cast<std::streamsize>(vault_bytes.size()));
  if (!out) {
    err = "Failed to write vault file";
    return false;
  }
  return true;
}

bool upsert(std::vector<Entry>& entries, const Entry& e) {
  for (auto& it : entries) {
    if (it.site == e.site) {
      it = e;
      return true;
    }
  }
  entries.push_back(e);
  return true;
}

bool remove(std::vector<Entry>& entries, const std::string& site) {
  for (auto it = entries.begin(); it != entries.end(); ++it) {
    if (it->site == site) {
      entries.erase(it);
      return true;
    }
  }
  return false;
}

const Entry* find(const std::vector<Entry>& entries, const std::string& site) {
  for (const auto& e : entries) {
    if (e.site == site) return &e;
  }
  return nullptr;
}

} // namespace password_db
