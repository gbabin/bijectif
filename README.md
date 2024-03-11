# Bijectif &mdash; Image titles in filenames

![screenshot](screenshot.png)

## Build

### Arch Linux

Use `PKGBUILD`.

### Debian / Ubuntu

```
sudo apt install cmake make g++ qt6-base-dev qt6-multimedia-dev qt6-tools-dev
cmake -DCMAKE_BUILD_TYPE=Release . -B build
cmake --build build
```

### Fedora

```
sudo dnf install cmake make gcc-c++ qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qttools-devel
cmake -DCMAKE_BUILD_TYPE=Release . -B build
cmake --build build
```

### openSUSE

```
sudo zypper install cmake make gcc-c++ qt6-base-devel qt6-multimedia-devel qt6-linguist-devel
cmake -DCMAKE_BUILD_TYPE=Release . -B build
cmake --build build
```

### Windows

Use `build-msys2.sh` in [MSYS2](https://www.msys2.org/) UCRT64 environment.

## Settings

Default `bijectif.ini`:

```
thumbnail size = 128
tooltip size = 512
min width = 800
min height = 600
max names = 8
max file size = 20971520
font size = 16
thumbnail cache size = 100
thumbnail database limit = 10000
log verbosity = 2
```

See [`settings.h`](src/settings.h).

## License

This project is licensed under the terms of the GNU General Public License v3.0. See `LICENSE`.
