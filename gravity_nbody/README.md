# gravity_nbody

2D N-body gravity simulation using SFML.

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
./gravity_nbody
```

## Controls

- `Space`: pause/resume
- `R`: random swarm preset
- `T`: toggle trails
- `C`: toggle color by speed
- `Z/X`: decrease/increase trail length
- `+/-`: increase/decrease gravity
- `[`/`]`: decrease/increase softening
- `,/.`: decrease/increase dt
- `1/2`: decrease/increase number of bodies
- `0`: reset view
- `F`: follow center of mass
- `F1/F2/F3`: galaxy/ring/swarm presets
- `S`: save config to `config.txt`
- `L`: load config from `config.txt`
- `Right Mouse Drag`: pan
- `Mouse Wheel`: zoom

## Config

Edit `config.txt` to set default values on startup.
