# secure_vault_pm

C++17 CLI for a secure file vault and a local password manager using OpenSSL.

## Features

- AES-256-GCM encryption with integrity (tag)
- PBKDF2-HMAC-SHA256 key derivation
- File encryption/decryption
- Encrypted password vault with CRUD

## Build

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```

## Usage

### File Vault

```bash
./build/secure_vault_pm vault encrypt <in_file> <out_file> --pass <password>
./build/secure_vault_pm vault decrypt <in_file> <out_file> --pass <password>
```

### Password Manager

```bash
./build/secure_vault_pm pm init <vault_file> --pass <password>
./build/secure_vault_pm pm add <vault_file> --pass <password> --site <site> --user <user> --passw <pass> --note <note>
./build/secure_vault_pm pm list <vault_file> --pass <password>
./build/secure_vault_pm pm get <vault_file> --pass <password> --site <site>
./build/secure_vault_pm pm delete <vault_file> --pass <password> --site <site>
```

## Vault File Format

- Magic: `SVPM`
- Version: `1`
- Salt: 16 bytes
- IV: 12 bytes
- Tag: 16 bytes
- Ciphertext length: 4 bytes (big-endian)
- Ciphertext

## Notes

- Do not roll your own crypto. This project uses OpenSSL for correctness.
- Keep your master password strong and unique.
