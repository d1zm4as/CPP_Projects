cmake_minimum_required(VERSION 3.16)
project(secure_vault_pm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

add_executable(secure_vault_pm
  src/main.cpp
  src/crypto.cpp
  src/vault.cpp
  src/password_db.cpp
)

target_include_directories(secure_vault_pm PRIVATE src)

target_link_libraries(secure_vault_pm PRIVATE OpenSSL::Crypto)
