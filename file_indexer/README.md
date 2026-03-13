# file_indexer

Lightweight CLI file indexer and search tool.

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Usage

Index a directory:

```bash
./file_indexer index /path/to/root index.txt
```

Search the index:

```bash
./file_indexer search index.txt report --limit 20
```

Filters:

```bash
./file_indexer search index.txt main --ext .cpp
./file_indexer search index.txt log --min-size 1000 --max-size 100000
./file_indexer search index.txt backup --modified-since 7
```

Show stats:

```bash
./file_indexer stats index.txt
```
