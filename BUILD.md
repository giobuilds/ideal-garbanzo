# Building Anno Clone on Fedora Linux

## 1. Install SDL3 development package

```bash
sudo dnf install SDL3-devel
```

If SDL3 is not in the Fedora repos for your version, build from source:

```bash
git clone https://github.com/libsdl-org/SDL.git --branch SDL3 --depth 1
cd SDL
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

## 2. Configure and build

```bash
cd anno_clone          # this project directory
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

For a release build (optimised, no debug symbols):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## 3. Run

```bash
./build/anno_clone
```

## Controls (Phase 1)

| Key              | Action          |
|------------------|-----------------|
| W / Arrow Up     | Pan camera up   |
| S / Arrow Down   | Pan camera down |
| A / Arrow Left   | Pan camera left |
| D / Arrow Right  | Pan camera right|
| Escape           | Quit            |
| Mouse move       | Highlight tile  |

## Troubleshooting

- **"SDL3 not found"** – make sure `SDL3-devel` is installed and re-run cmake.
- **Black screen** – check `SDL_Log` output in the terminal; renderer errors are printed there.
- **Segfault at startup** – run under `gdb ./build/anno_clone` and check the backtrace with `bt`.
