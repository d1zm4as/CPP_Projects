cmake_minimum_required(VERSION 3.16)
project(mini_shell LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(mini_shell
    src/main.cpp
)

find_library(READLINE_LIBRARY readline)
find_library(NCURSES_LIBRARY ncurses)

if (NOT READLINE_LIBRARY)
    message(FATAL_ERROR "readline library not found. Install libreadline-dev.")
endif()

target_link_libraries(mini_shell PRIVATE ${READLINE_LIBRARY})
if (NCURSES_LIBRARY)
    target_link_libraries(mini_shell PRIVATE ${NCURSES_LIBRARY})
endif()
