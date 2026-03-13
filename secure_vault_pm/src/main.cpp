#include "password_db.hpp"
#include "vault.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

void print_usage() {
  std::cout << "Usage:\n"
            << "  secure_vault_pm vault encrypt <in> <out> --pass <password>\n"
            << "  secure_vault_pm vault decrypt <in> <out> --pass <password>\n"
            << "  secure_vault_pm pm init <vault> --pass <password>\n"
            << "  secure_vault_pm pm add <vault> --pass <password> --site <site> --user <user> --passw <pass> --note <note>\n"
            << "  secure_vault_pm pm list <vault> --pass <password>\n"
            << "  secure_vault_pm pm get <vault> --pass <password> --site <site>\n"
            << "  secure_vault_pm pm delete <vault> --pass <password> --site <site>\n";
}

bool get_flag_value(const std::vector<std::string>& args, const std::string& flag, std::string& out) {
  for (size_t i = 0; i + 1 < args.size(); ++i) {
    if (args[i] == flag) {
      out = args[i + 1];
      return true;
    }
  }
  return false;
}

} // namespace

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  if (args.size() < 1) {
    print_usage();
    return 1;
  }

  const std::string& mode = args[0];
  if (mode == "vault") {
    if (args.size() < 5) {
      print_usage();
      return 1;
    }
    const std::string& action = args[1];
    const std::string& in_path = args[2];
    const std::string& out_path = args[3];
    std::string password;
    if (!get_flag_value(args, "--pass", password)) {
      std::cerr << "Missing --pass" << std::endl;
      return 1;
    }
    std::string err;
    if (action == "encrypt") {
      if (!vault::encrypt_file(in_path, out_path, password, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      std::cout << "Encrypted to " << out_path << std::endl;
      return 0;
    }
    if (action == "decrypt") {
      if (!vault::decrypt_file(in_path, out_path, password, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      std::cout << "Decrypted to " << out_path << std::endl;
      return 0;
    }
    print_usage();
    return 1;
  }

  if (mode == "pm") {
    if (args.size() < 3) {
      print_usage();
      return 1;
    }
    const std::string& action = args[1];
    const std::string& vault_path = args[2];
    std::string master_pass;
    if (!get_flag_value(args, "--pass", master_pass)) {
      std::cerr << "Missing --pass" << std::endl;
      return 1;
    }

    std::string err;

    if (action == "init") {
      std::vector<password_db::Entry> entries;
      if (!password_db::save(vault_path, master_pass, entries, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      std::cout << "Vault initialized: " << vault_path << std::endl;
      return 0;
    }

    if (action == "add") {
      std::string site, user, passw, note;
      if (!get_flag_value(args, "--site", site) ||
          !get_flag_value(args, "--user", user) ||
          !get_flag_value(args, "--passw", passw) ||
          !get_flag_value(args, "--note", note)) {
        std::cerr << "Missing required fields" << std::endl;
        return 1;
      }

      std::vector<password_db::Entry> entries;
      if (!password_db::load(vault_path, master_pass, entries, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      password_db::Entry e{site, user, passw, note};
      password_db::upsert(entries, e);
      if (!password_db::save(vault_path, master_pass, entries, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      std::cout << "Entry saved for site: " << site << std::endl;
      return 0;
    }

    if (action == "list") {
      std::vector<password_db::Entry> entries;
      if (!password_db::load(vault_path, master_pass, entries, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      for (const auto& e : entries) {
        std::cout << e.site << "\t" << e.user << "\t" << e.note << std::endl;
      }
      return 0;
    }

    if (action == "get") {
      std::string site;
      if (!get_flag_value(args, "--site", site)) {
        std::cerr << "Missing --site" << std::endl;
        return 1;
      }
      std::vector<password_db::Entry> entries;
      if (!password_db::load(vault_path, master_pass, entries, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      const auto* e = password_db::find(entries, site);
      if (!e) {
        std::cerr << "Not found" << std::endl;
        return 1;
      }
      std::cout << "site=" << e->site << "\n"
                << "user=" << e->user << "\n"
                << "pass=" << e->pass << "\n"
                << "note=" << e->note << std::endl;
      return 0;
    }

    if (action == "delete") {
      std::string site;
      if (!get_flag_value(args, "--site", site)) {
        std::cerr << "Missing --site" << std::endl;
        return 1;
      }
      std::vector<password_db::Entry> entries;
      if (!password_db::load(vault_path, master_pass, entries, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      if (!password_db::remove(entries, site)) {
        std::cerr << "Not found" << std::endl;
        return 1;
      }
      if (!password_db::save(vault_path, master_pass, entries, err)) {
        std::cerr << "Error: " << err << std::endl;
        return 1;
      }
      std::cout << "Deleted site: " << site << std::endl;
      return 0;
    }

    print_usage();
    return 1;
  }

  print_usage();
  return 1;
}
