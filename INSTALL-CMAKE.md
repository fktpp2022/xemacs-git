# Building XEmacs with CMake

This document describes how to build XEmacs using CMake as an alternative to the traditional autoconf/configure build system.

## Overview

XEmacs has been migrated from autoconf to CMake. This provides:

- Modern cross-platform build system
- Out-of-source builds
- Better IDE integration
- CPack packaging support
- Faster configuration times

## Prerequisites

### Required Tools

- CMake 3.16 or later
- C compiler (GCC, Clang, MSVC, etc.)
- GNU Make (or Ninja, or other CMake-supported build tool)

### Required Libraries

- X11 development libraries (for X11 GUI support)
  - libX11, libXt, libXmu, libXpm, libXext, etc.
- GTK+ 2 or 3 (for GTK GUI support)
- Image libraries (optional but recommended):
  - libpng, libjpeg, libtiff, libgif, libxpm
- Database libraries (optional):
  - libdb (Berkeley DB), libgdbm, libldap, libpq (PostgreSQL)
- Sound libraries (optional):
  - ALSA, OSS, NAS, ESD
- Other libraries:
  - zlib, ncurses, openssl (for TLS), gmp (for bignum)

## Quick Start

### Basic Build

```bash
mkdir build
cd build
cmake ..
make xemacs    # compile C code and link the raw binary
make dump      # byte-compile lisp files and produce the pdump
```

After `make dump`, the working binary is at `bin/xemacs` with its dump
file `bin/xemacs.dmp`.

### Install

```bash
sudo make install
```

### Out-of-Source Build (Recommended)

```bash
mkdir -p ~/builds/xemacs
cd ~/builds/xemacs
cmake /path/to/xemacs/source
make xemacs
make dump
sudo make install
```

### Build Flow

The cmake build separates compilation from dumping, unlike autoconf's
`make` which does everything in one step:

| Step | Command | What it does |
|------|---------|--------------|
| 1. Configure | `cmake ..` | Detect features, generate Makefiles |
| 2. Compile | `make xemacs` | Compile C code, link `bin/xemacs` |
| 3. Dump | `make dump` | Byte-compile `.el` → `.elc`, then pdump |

The `make dump` step runs the raw binary to:
1. Byte-compile all Lisp files needed for dumping (`update-elc`)
2. Load the compiled Lisp and dump the heap to `xemacs.dmp` (`loadup.el dump`)
3. Copy the dump file next to the binary

This matches autoconf's internal flow (`NEEDTODUMP` → dump → `update-elc-2`)
but exposes it as an explicit build target.

## Configuration Options

### CMake Standard Options

#### Build Type

```bash
# Debug build (includes debug symbols, no optimization)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (full optimization)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Release with debug info (default)
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# Minimal size release
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..
```

#### Installation Prefix

```bash
# Install to /usr (instead of /usr/local)
cmake -DCMAKE_INSTALL_PREFIX=/usr ..

# Install to custom location
cmake -DCMAKE_INSTALL_PREFIX=/opt/xemacs ..
```

### XEmacs-Specific Options

#### Window System Options

```bash
# Enable X11 support (default: ON)
cmake -DXEMACS_WITH_X11=ON ..

# Enable GTK support (default: OFF)
cmake -DXEMACS_WITH_GTK=ON ..

# Enable GNOME support (default: OFF)
cmake -DXEMACS_WITH_GNOME=ON ..
```

#### Image Format Options

```bash
# Enable PNG support (default: ON)
cmake -DXEMACS_WITH_PNG=ON ..

# Enable JPEG support (default: ON)
cmake -DXEMACS_WITH_JPEG=ON ..

# Enable TIFF support (default: ON)
cmake -DXEMACS_WITH_TIFF=ON ..

# Enable GIF support (default: ON)
cmake -DXEMACS_WITH_GIF=ON ..

# Enable XPM support (default: ON)
cmake -DXEMACS_WITH_XPM=ON ..

# Enable X-Face support (default: OFF)
cmake -DXEMACS_WITH_XFACE=ON ..
```

#### Sound Options

```bash
# Enable sound support (default: ON)
cmake -DXEMACS_WITH_SOUND=ON ..

# Enable ALSA sound (default: ON)
cmake -DXEMACS_WITH_ALSA=ON ..

# Enable OSS sound (default: ON)
cmake -DXEMACS_WITH_OSS=ON ..

# Enable NAS sound (default: OFF)
cmake -DXEMACS_WITH_NAS=ON ..

# Enable ESD sound (default: OFF)
cmake -DXEMACS_WITH_ESD=ON ..
```

#### Database Options

```bash
# Enable database support (default: ON)
cmake -DXEMACS_WITH_DATABASE=ON ..

# Enable Berkeley DB (default: ON)
cmake -DXEMACS_WITH_BERKELEY_DB=ON ..

# Enable GDBM (default: ON)
cmake -DXEMACS_WITH_GDBM=ON ..

# Enable LDAP (default: ON)
cmake -DXEMACS_WITH_LDAP=ON ..

# Enable PostgreSQL (default: ON)
cmake -DXEMACS_WITH_POSTGRESQL=ON ..
```

#### Input Method Options

```bash
# Enable XIM (default: ON)
cmake -DXEMACS_WITH_XIM=ON ..

# Enable Canna (default: OFF)
cmake -DXEMACS_WITH_CANNA=ON ..

# Enable Wnn (default: OFF)
cmake -DXEMACS_WITH_WNN=ON ..

# Enable Wnn6 (default: OFF)
cmake -DXEMACS_WITH_WNN6=ON ..
```

#### GUI Widget Options

```bash
# Enable menubars (default: ON)
cmake -DXEMACS_WITH_MENUBARS=ON ..

# Enable scrollbars (default: ON)
cmake -DXEMACS_WITH_SCROLLBARS=ON ..

# Enable dialogs (default: ON)
cmake -DXEMACS_WITH_DIALOGS=ON ..

# Enable toolbars (default: ON)
cmake -DXEMACS_WITH_TOOLBARS=ON ..

# Enable widgets (default: OFF)
cmake -DXEMACS_WITH_WIDGETS=ON ..

# Widget type selection (lucid, motif, athena, gtk, msw)
cmake -DXEMACS_WITH_MENUBARS_TYPE=lucid ..
cmake -DXEMACS_WITH_SCROLLBARS_TYPE=lucid ..
cmake -DXEMACS_WITH_DIALOGS_TYPE=lucid ..
cmake -DXEMACS_WITH_WIDGETS_TYPE=lucid ..
```

#### TTY Options

```bash
# Enable TTY support (default: ON)
cmake -DXEMACS_WITH_TTY=ON ..

# Enable ncurses (default: ON)
cmake -DXEMACS_WITH_NCURSES=ON ..

# Enable GPM mouse (default: OFF)
cmake -DXEMACS_WITH_GPM=ON ..
```

#### Feature Options

```bash
# Enable Xft font rendering (default: OFF)
cmake -DXEMACS_WITH_XFT=ON ..

# Enable fontconfig (default: OFF)
cmake -DXEMACS_WITH_FONTCONFIG=ON ..

# Enable X FontSet (default: OFF)
cmake -DXEMACS_WITH_XFS=ON ..

# Enable zlib compression (default: ON)
cmake -DXEMACS_WITH_ZLIB=ON ..

# Enable dynamic modules (default: ON)
cmake -DXEMACS_WITH_MODULES=ON ..

# Enable TLS/SSL (default: ON)
cmake -DXEMACS_WITH_TLS=ON ..

# Enable bignum support (default: OFF)
cmake -DXEMACS_WITH_BIGNUM=ON ..
```

#### Debug and Build Options

```bash
# Enable debug build (default: ON)
cmake -DXEMACS_WITH_DEBUG=ON ..

# Enable runtime error checking (default: ON)
cmake -DXEMACS_WITH_ERROR_CHECKING=ON ..

# Enable runtime assertions (default: ON)
cmake -DXEMACS_WITH_ASSERTIONS=ON ..

# Enable memory usage stats (default: ON)
cmake -DXEMACS_WITH_MEMORY_USAGE_STATS=ON ..

# Enable quick build (for development) (default: OFF)
cmake -DXEMACS_WITH_QUICK_BUILD=ON ..

# Enable dumping into executable (default: ON)
cmake -DXEMACS_WITH_DUMP_IN_EXEC=ON ..

# Enable relocating allocator (default: OFF)
cmake -DXEMACS_WITH_REL_ALLOC=ON ..
```

#### Installation Options

```bash
# Install documentation (default: ON)
cmake -DXEMACS_INSTALL_DOC=ON ..

# Install Lisp files (default: ON)
cmake -DXEMACS_INSTALL_LISP=ON ..

# Install etc files (default: ON)
cmake -DXEMACS_INSTALL_ETC=ON ..

# Install info files (default: ON)
cmake -DXEMACS_INSTALL_INFO=ON ..

# Enable site-lisp (default: ON)
cmake -DXEMACS_WITH_SITE_LISP=ON ..

# Enable site-modules (default: ON)
cmake -DXEMACS_WITH_SITE_MODULES=ON ..
```

#### Mail Locking

```bash
# Mail locking method (lockf, flock, file, locking, mmdf, pop)
cmake -DXEMACS_MAIL_LOCKING=file ..
```

## Using ccmake or cmake-gui

For interactive configuration, you can use `ccmake` (curses-based) or `cmake-gui` (Qt-based):

```bash
# Curses-based interface
ccmake ..

# Qt GUI interface
cmake-gui ..
```

These tools provide a more user-friendly way to explore and set all available options.

## Installation Directories

### Default Locations

| Component | Default Path |
|-----------|--------------|
| Prefix | `/usr/local` |
| Binaries | `/usr/local/bin` |
| Lisp files | `/usr/local/share/xemacs-<version>/lisp` |
| Data files | `/usr/local/share/xemacs-<version>/etc` |
| Libraries | `/usr/local/lib/xemacs-<version>/<arch>` |
| Modules | `/usr/local/lib/xemacs-<version>/<arch>/modules` |
| Info files | `/usr/local/share/xemacs-<version>/info` |
| Man pages | `/usr/local/share/man/man1` |
| Site-lisp | `/usr/local/share/xemacs/site-lisp` |
| Site-modules | `/usr/local/lib/xemacs/site-modules` |

### Customizing Directories

You can customize individual installation directories:

```bash
cmake \
  -DCMAKE_INSTALL_PREFIX=/opt/xemacs \
  -DXEMACS_BINDIR=/opt/xemacs/bin \
  -DXEMACS_DATADIR=/opt/xemacs/share \
  -DXEMACS_LIBDIR=/opt/xemacs/lib \
  -DXEMACS_MANDIR=/opt/xemacs/man \
  ..
```

## Packaging with CPack

CMake provides CPack for creating installable packages.

### Creating Packages

```bash
# Create all package types
make package

# Create specific package types
cpack -G TGZ
cpack -G TBZ2
cpack -G ZIP

# On Debian/Ubuntu systems
cpack -G DEB

# On Red Hat/Fedora systems
cpack -G RPM

# On macOS
cpack -G DragNDrop

# On Windows
cpack -G NSIS
```

### Available Generators

| Generator | Description |
|-----------|-------------|
| `TGZ` | Gzip-compressed tar archive |
| `TBZ2` | Bzip2-compressed tar archive |
| `ZIP` | ZIP archive |
| `DEB` | Debian package (.deb) |
| `RPM` | RPM package (.rpm) |
| `DragNDrop` | macOS disk image (.dmg) |
| `NSIS` | Windows installer |

## Configuration Summary

After running `cmake`, a configuration summary is displayed showing:

- XEmacs version
- Installation prefix
- Window system support (X11, GTK, MS Windows)
- Image format support (PNG, JPEG, TIFF, GIF, XPM, X-Face)
- Sound support
- Database support
- Input methods
- Widget types
- Debug and build options

### Installation File

CMake generates an `Installation.txt` file in the build directory (equivalent to the traditional `Installation` file). This file contains:

- Complete configuration summary
- All detected features
- Installation paths
- Build options

After installation, this file is installed as `Installation` in the documentation directory.

### CMakeCache.txt

All configuration options are stored in `CMakeCache.txt` in the build directory. You can:

- Review the file to see all settings
- Edit it to change options (advanced)
- Delete it to start fresh configuration

## Building with Different Generators

### Makefiles (Default)

```bash
cmake ..
make -j4
sudo make install
```

### Ninja

```bash
cmake -G Ninja ..
ninja
sudo ninja install
```

### Xcode (macOS)

```bash
cmake -G Xcode ..
# Open Xcode and build, or use:
xcodebuild -project XEmacs.xcodeproj -configuration Release
```

### Visual Studio (Windows)

```bash
cmake -G "Visual Studio 17 2022" ..
# Open the solution in Visual Studio, or use:
cmake --build . --config Release
```

## Multiple Build Configurations

You can maintain multiple build directories with different configurations:

```bash
# Debug build
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug -DXEMACS_WITH_DEBUG=ON ../..
make

# Release build
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release -DXEMACS_WITH_DEBUG=OFF ../..
make

# GTK build
mkdir -p build/gtk
cd build/gtk
cmake -DXEMACS_WITH_GTK=ON -DXEMACS_WITH_X11=OFF ../..
make
```

## Troubleshooting

### Common Issues

1. **CMake version too old**
   ```bash
   cmake --version
   # Install or upgrade CMake
   ```

2. **Missing libraries**
   ```bash
   # On Debian/Ubuntu
   sudo apt-get install libx11-dev libxt-dev libxmu-dev libxpm-dev
   sudo apt-get install libpng-dev libjpeg-dev libtiff-dev libgif-dev
   
   # On Red Hat/Fedora
   sudo dnf install libX11-devel libXt-devel libXmu-devel libXpm-devel
   sudo dnf install libpng-devel libjpeg-devel libtiff-devel giflib-devel
   
   # On macOS (with Homebrew)
   brew install libpng jpeg libtiff giflib
   ```

3. **Configuration fails**
   - Check `CMakeCache.txt` for clues
   - Look at `CMakeFiles/CMakeError.log` and `CMakeFiles/CMakeOutput.log`
   - Use `cmake --trace` for detailed debugging

4. **Build fails**
   - Ensure all dependencies are installed
   - Check if you're using a supported compiler
   - Try a clean build (delete build directory and start over)

### Getting Help

- Check the `CMakeLists.txt` files for available options
- Use `ccmake ..` for interactive exploration
- Look at `cmake --help-variable-list` for standard CMake variables
- Check the XEmacs documentation in the `etc/` directory

## Comparison with Autoconf

### Equivalent Commands

| Autoconf | CMake |
|----------|-------|
| `./configure` | `cmake ..` |
| `./configure --prefix=/usr` | `cmake -DCMAKE_INSTALL_PREFIX=/usr ..` |
| `./configure --with-x11` | `cmake -DXEMACS_WITH_X11=ON ..` |
| `./configure --without-gtk` | `cmake -DXEMACS_WITH_GTK=OFF ..` |
| `make` | `make xemacs && make dump` |
| `make install` | `make install` or `cmake --build . --target install` |
| `make distclean` | `rm -rf build/` |

### Key Differences

1. **Separate dump step**: Autoconf's `make` compiles, byte-compiles, and
   dumps in one invocation.  CMake splits this into `make xemacs` (compile)
   and `make dump` (byte-compile + pdump).  This is because CMake custom
   commands cannot easily replicate the autoconf Makefile's recursive
   self-invocation pattern.

2. **Out-of-source builds**: CMake requires or strongly encourages
   out-of-source builds, keeping the source tree clean.  Note that
   byte-compiled `.elc` files are written into the source `lisp/` directory
   (same as autoconf).

3. **Configuration persistence**: All options are stored in `CMakeCache.txt`,
   so you don't need to remember them for subsequent builds.

4. **Multiple generators**: CMake can generate Makefiles, Ninja files,
   Xcode projects, Visual Studio solutions, etc.

5. **CPack integration**: Packaging is built into the system with CPack.

## Development Tips

### Regenerating Build Files

If you modify `CMakeLists.txt`:

```bash
# CMake will automatically reconfigure on next build
make

# Or force reconfiguration
cmake ..
```

### Adding New Source Files

Edit the appropriate `CMakeLists.txt`:

- `src/CMakeLists.txt` for core source files
- `lib-src/CMakeLists.txt` for utility programs
- `lwlib/CMakeLists.txt` for widget library
- `modules/CMakeLists.txt` for dynamic modules

### Verbose Build

```bash
# Show all compiler commands
make VERBOSE=1

# Or with cmake
cmake --build . --verbose
```

### Installing to Staging Directory

```bash
# Useful for package creation
make DESTDIR=/tmp/staging install
```

## Further Reading

- [CMake Documentation](https://cmake.org/documentation/)
- [CPack Documentation](https://cmake.org/cmake/help/latest/manual/cpack.1.html)
- XEmacs `INSTALL` file (traditional autoconf instructions)
- `CMakeLists.txt` files in the source tree for detailed options
