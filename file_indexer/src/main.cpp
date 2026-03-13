#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

struct Entry {
    std::string path;
    std::string name;
    std::string ext;
    uintmax_t size = 0;
    std::time_t mtime = 0;
};

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static std::string trim(std::string s) {
    auto is_space = [](unsigned char c) { return std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&](unsigned char c) { return !is_space(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [&](unsigned char c) { return !is_space(c); }).base(), s.end());
    return s;
}

static std::vector<std::string> split_tokens(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> out;
    std::string tok;
    while (iss >> tok) out.push_back(tok);
    return out;
}

static void save_index(const std::string& path, const std::vector<Entry>& entries) {
    std::ofstream out(path);
    for (const auto& e : entries) {
        out << e.path << "\t" << e.name << "\t" << e.ext << "\t" << e.size << "\t" << e.mtime << "\n";
    }
}

static std::vector<Entry> load_index(const std::string& path) {
    std::vector<Entry> entries;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        Entry e;
        std::string size_str;
        std::string mtime_str;
        if (!std::getline(iss, e.path, '\t')) continue;
        if (!std::getline(iss, e.name, '\t')) continue;
        if (!std::getline(iss, e.ext, '\t')) continue;
        if (!std::getline(iss, size_str, '\t')) continue;
        if (!std::getline(iss, mtime_str, '\t')) continue;
        e.size = static_cast<uintmax_t>(std::stoull(size_str));
        e.mtime = static_cast<std::time_t>(std::stoll(mtime_str));
        entries.push_back(e);
    }
    return entries;
}

static void build_index(const fs::path& root, std::vector<Entry>& entries) {
    entries.clear();
    for (auto it = fs::recursive_directory_iterator(root, fs::directory_options::skip_permission_denied); it != fs::recursive_directory_iterator(); ++it) {
        std::error_code ec;
        if (!it->is_regular_file(ec)) continue;
        const auto& p = it->path();
        Entry e;
        e.path = p.string();
        e.name = p.filename().string();
        e.ext = to_lower(p.extension().string());
        e.size = fs::file_size(p, ec);
        if (ec) e.size = 0;
        auto ftime = fs::last_write_time(p, ec);
        if (!ec) {
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            e.mtime = std::chrono::system_clock::to_time_t(sctp);
        } else {
            e.mtime = 0;
        }
        entries.push_back(std::move(e));
    }
}

static void print_results(const std::vector<Entry>& results, size_t limit) {
    size_t count = 0;
    for (const auto& e : results) {
        std::cout << e.path << " (" << e.size << " bytes)\n";
        if (++count >= limit) break;
    }
    if (results.empty()) std::cout << "No matches.\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "File Indexer\n";
        std::cout << "Commands:\n";
        std::cout << "  index <root> <index_file>\n";
        std::cout << "  search <index_file> <query> [--limit N] [--ext .cpp] [--min-size B] [--max-size B] [--modified-since DAYS]\n";
        std::cout << "  stats <index_file>\n";
        return 0;
    }

    std::string cmd = argv[1];
    if (cmd == "index") {
        if (argc < 4) {
            std::cerr << "Usage: index <root> <index_file>\n";
            return 1;
        }
        fs::path root = argv[2];
        std::string index_file = argv[3];
        std::vector<Entry> entries;
        build_index(root, entries);
        save_index(index_file, entries);
        std::cout << "Indexed " << entries.size() << " files.\n";
        return 0;
    }

    if (cmd == "stats") {
        if (argc < 3) {
            std::cerr << "Usage: stats <index_file>\n";
            return 1;
        }
        auto entries = load_index(argv[2]);
        uintmax_t total = 0;
        std::unordered_map<std::string, size_t> by_ext;
        for (const auto& e : entries) {
            total += e.size;
            by_ext[e.ext]++;
        }
        std::cout << "Files: " << entries.size() << "\n";
        std::cout << "Total size: " << total << " bytes\n";
        std::cout << "By extension:\n";
        for (const auto& kv : by_ext) {
            std::cout << "  " << (kv.first.empty() ? "<none>" : kv.first) << ": " << kv.second << "\n";
        }
        return 0;
    }

    if (cmd == "search") {
        if (argc < 4) {
            std::cerr << "Usage: search <index_file> <query> [--limit N] [--ext .cpp] [--min-size B] [--max-size B] [--modified-since DAYS]\n";
            return 1;
        }
        std::string index_file = argv[2];
        std::string query = argv[3];
        size_t limit = 50;
        std::string ext_filter;
        uintmax_t min_size = 0;
        uintmax_t max_size = std::numeric_limits<uintmax_t>::max();
        int modified_days = -1;

        for (int i = 4; i + 1 < argc; ++i) {
            std::string key = argv[i];
            std::string val = argv[i + 1];
            if (key == "--limit") limit = static_cast<size_t>(std::stoul(val));
            else if (key == "--ext") ext_filter = to_lower(val);
            else if (key == "--min-size") min_size = static_cast<uintmax_t>(std::stoull(val));
            else if (key == "--max-size") max_size = static_cast<uintmax_t>(std::stoull(val));
            else if (key == "--modified-since") modified_days = std::stoi(val);
        }

        auto entries = load_index(index_file);
        std::string q = to_lower(query);
        std::vector<Entry> results;
        results.reserve(entries.size());

        std::time_t now = std::time(nullptr);
        std::time_t cutoff = 0;
        if (modified_days >= 0) cutoff = now - static_cast<std::time_t>(modified_days) * 24 * 60 * 60;

        for (const auto& e : entries) {
            std::string name = to_lower(e.name);
            std::string path = to_lower(e.path);
            if (name.find(q) == std::string::npos && path.find(q) == std::string::npos) continue;
            if (!ext_filter.empty() && e.ext != ext_filter) continue;
            if (e.size < min_size || e.size > max_size) continue;
            if (modified_days >= 0 && e.mtime < cutoff) continue;
            results.push_back(e);
        }

        print_results(results, limit);
        return 0;
    }

    std::cerr << "Unknown command.\n";
    return 1;
}
