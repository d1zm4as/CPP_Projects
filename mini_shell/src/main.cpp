#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/history.h>
#include <readline/readline.h>

static std::vector<std::string> split_args(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> args;
    std::string token;
    while (iss >> token) args.push_back(token);
    return args;
}

static std::vector<char*> to_argv(std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto& s : args) argv.push_back(s.data());
    argv.push_back(nullptr);
    return argv;
}

static void run_command(std::vector<std::string> args) {
    if (args.empty()) return;

    if (args[0] == "cd") {
        const char* path = (args.size() > 1) ? args[1].c_str() : std::getenv("HOME");
        if (chdir(path) != 0) {
            std::cerr << "cd: " << std::strerror(errno) << "\n";
        }
        return;
    }

    if (args[0] == "pwd") {
        char buf[4096];
        if (getcwd(buf, sizeof(buf))) std::cout << buf << "\n";
        else std::cerr << "pwd: " << std::strerror(errno) << "\n";
        return;
    }

    if (args[0] == "exit") {
        std::exit(0);
    }

    pid_t pid = fork();
    if (pid == 0) {
        auto argv = to_argv(args);
        execvp(argv[0], argv.data());
        std::cerr << "exec: " << std::strerror(errno) << "\n";
        std::_Exit(127);
    } else if (pid > 0) {
        int status = 0;
        waitpid(pid, &status, 0);
    } else {
        std::cerr << "fork: " << std::strerror(errno) << "\n";
    }
}

static const char* builtins[] = {"cd", "pwd", "exit", nullptr};

static char* dupstr(const char* s) {
    size_t len = std::strlen(s) + 1;
    char* out = static_cast<char*>(std::malloc(len));
    if (out) std::memcpy(out, s, len);
    return out;
}

static char* builtin_generator(const char* text, int state) {
    static int list_index;
    static size_t len;
    if (state == 0) {
        list_index = 0;
        len = std::strlen(text);
    }
    const char* name = nullptr;
    while ((name = builtins[list_index++])) {
        if (std::strncmp(name, text, len) == 0) return dupstr(name);
    }
    return nullptr;
}

static char** completion(const char* text, int start, int end) {
    (void)end;
    if (start == 0) {
        return rl_completion_matches(text, builtin_generator);
    }
    return rl_completion_matches(text, rl_filename_completion_function);
}

int main() {
    rl_attempted_completion_function = completion;

    while (true) {
        char* input = readline("mini-shell> ");
        if (!input) break;
        if (*input) add_history(input);
        std::string line(input);
        std::free(input);
        auto args = split_args(line);
        run_command(args);
    }

    return 0;
}
