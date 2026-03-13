# http_server

Minimal multi-threaded HTTP server in C++17.

## Features

- Multi-threaded (one thread per connection)
- Static file serving from `www/`
- Simple routes: `/health`, `/time`, `/echo?msg=...`

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Run

```bash
./http_server 8080 ../www
```

Then open:
- `http://localhost:8080/`
- `http://localhost:8080/health`
- `http://localhost:8080/time`
- `http://localhost:8080/echo?msg=hello`
