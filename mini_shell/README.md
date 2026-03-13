# mini_shell

Minimal Unix-like shell in C++.

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Run

```bash
./mini_shell
```

## Built-ins

- `cd [path]`
- `pwd`
- `exit`

## Features

- Command history (up/down)
- Tab completion (built-ins + filesystem paths)
