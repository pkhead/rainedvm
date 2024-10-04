# Rained Version Manager
Version manager for [Rained](https://github.com/pkhead/Rained)

## Building
Prerequisities:
- C/C++ compiler 
- Meson build system

1. Clone from GitHub:
```bash
git clone --recursive https://github.com/pkhead/rainedvm
```

2. Install dependencies
```bash
# debian/ubuntu
sudo apt install libx11-dev libxkbcommon-dev xorg-dev libcurl4-openssl-dev

# windows - will use meson wrap
```

3. Uncomment `#define IMGUI_ENABLE_FREETYPE` in the file [imgui/imconfig.h](imgui/imconfig.h)
(I don't know how to make the repository track this, lol)

4. Setup Meson build directory (first-time only):
```bash
meson setup builddir
```

5. Compile and run:
```bash
meson compile -C builddir
builddir/rainedvm
```
