#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

struct Request {
    std::string method;
    std::string path;
    std::string query;
    std::map<std::string, std::string> headers;
};

static std::mutex log_mutex;

static void log_line(const std::string& line) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::cout << line << std::endl;
}

static std::string http_date() {
    std::time_t t = std::time(nullptr);
    char buf[128];
    std::tm tm = *std::gmtime(&t);
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return buf;
}

static std::string url_decode(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            int v = 0;
            std::istringstream iss(in.substr(i + 1, 2));
            iss >> std::hex >> v;
            out.push_back(static_cast<char>(v));
            i += 2;
        } else if (in[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(in[i]);
        }
    }
    return out;
}

static std::map<std::string, std::string> parse_query(const std::string& q) {
    std::map<std::string, std::string> out;
    std::istringstream iss(q);
    std::string part;
    while (std::getline(iss, part, '&')) {
        auto eq = part.find('=');
        if (eq == std::string::npos) continue;
        std::string key = url_decode(part.substr(0, eq));
        std::string val = url_decode(part.substr(eq + 1));
        out[key] = val;
    }
    return out;
}

static bool read_request(int fd, Request& req) {
    std::string data;
    char buf[4096];
    while (data.find("\r\n\r\n") == std::string::npos) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) return false;
        data.append(buf, buf + n);
        if (data.size() > 64 * 1024) return false;
    }

    std::istringstream iss(data);
    std::string line;
    if (!std::getline(iss, line)) return false;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream first(line);
    std::string target;
    first >> req.method >> target;
    if (req.method.empty() || target.empty()) return false;

    auto qpos = target.find('?');
    if (qpos == std::string::npos) {
        req.path = target;
    } else {
        req.path = target.substr(0, qpos);
        req.query = target.substr(qpos + 1);
    }

    while (std::getline(iss, line)) {
        if (line == "\r" || line.empty()) break;
        if (line.back() == '\r') line.pop_back();
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && value.front() == ' ') value.erase(value.begin());
        req.headers[key] = value;
    }

    return true;
}

static std::string mime_type(const fs::path& p) {
    auto ext = p.extension().string();
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".txt") return "text/plain";
    return "application/octet-stream";
}

static void send_response(int fd, const std::string& status, const std::string& body, const std::string& type) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << "\r\n";
    oss << "Date: " << http_date() << "\r\n";
    oss << "Content-Type: " << type << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    std::string header = oss.str();
    send(fd, header.data(), header.size(), 0);
    send(fd, body.data(), body.size(), 0);
}

static void handle_client(int fd, fs::path www_root) {
    Request req;
    if (!read_request(fd, req)) {
        close(fd);
        return;
    }

    log_line(req.method + " " + req.path);

    if (req.method != "GET") {
        send_response(fd, "405 Method Not Allowed", "Method Not Allowed", "text/plain");
        close(fd);
        return;
    }

    if (req.path == "/health") {
        send_response(fd, "200 OK", "OK", "text/plain");
        close(fd);
        return;
    }

    if (req.path == "/time") {
        send_response(fd, "200 OK", http_date(), "text/plain");
        close(fd);
        return;
    }

    if (req.path == "/echo") {
        auto q = parse_query(req.query);
        auto it = q.find("msg");
        std::string msg = (it != q.end()) ? it->second : "";
        send_response(fd, "200 OK", msg, "text/plain");
        close(fd);
        return;
    }

    // Static file serving
    fs::path rel = req.path;
    if (rel == "/") rel = "/index.html";
    fs::path full = www_root / rel.relative_path();

    std::error_code ec;
    full = fs::weakly_canonical(full, ec);
    fs::path canonical_root = fs::weakly_canonical(www_root, ec);
    if (ec || full.string().find(canonical_root.string()) != 0) {
        send_response(fd, "403 Forbidden", "Forbidden", "text/plain");
        close(fd);
        return;
    }

    std::ifstream file(full, std::ios::binary);
    if (!file) {
        send_response(fd, "404 Not Found", "Not Found", "text/plain");
        close(fd);
        return;
    }

    std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    send_response(fd, "200 OK", body, mime_type(full));
    close(fd);
}

int main(int argc, char** argv) {
    int port = 8080;
    fs::path www_root = fs::current_path() / "www";

    if (argc >= 2) port = std::stoi(argv[1]);
    if (argc >= 3) www_root = argv[2];

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "socket failed\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind failed\n";
        return 1;
    }

    if (listen(server_fd, 64) < 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    log_line("HTTP server listening on port " + std::to_string(port));
    log_line("Serving root: " + www_root.string());

    while (true) {
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client), &len);
        if (client_fd < 0) continue;

        std::thread([client_fd, www_root]() {
            handle_client(client_fd, www_root);
        }).detach();
    }

    close(server_fd);
    return 0;
}
