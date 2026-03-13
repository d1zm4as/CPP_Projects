# atoms_md

Simple 2D molecular dynamics simulation using a Lennard-Jones potential.

## Install SFML (Linux)

- Debian/Ubuntu: `sudo apt install libsfml-dev`
- Fedora: `sudo dnf install SFML-devel`
- Arch: `sudo pacman -S sfml`
- openSUSE: `sudo zypper install sfml2-devel`

## Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Run

```bash
./atoms_md
```
