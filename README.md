# Rained Version Manager
Version manager for [Rained](https://github.com/pkhead/Rained)

> [!note]
> If attempting to run the executable fails, you might need to install the
> [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

## Building
Prerequisities:
- C and C++17 compiler 
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

3. Setup Meson build directory (first-time only):
```bash
meson setup builddir
```

4. Compile and run:
```bash
meson compile -C builddir
builddir/rainedvm
```
