# stargatetc-hlsdk-portable [![Build Status](https://github.com/jr551/stargatetc-hlsdk-portable/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/jr551/stargatetc-hlsdk-portable/actions/workflows/build.yml)

**Live** — active development. **This is the canonical repo for StargateTC game-logic work.**

StargateTC is a Stargate-themed total-conversion mod built on the Half-Life (GoldSrc) engine. This fork of [FWGS/hlsdk-portable](https://github.com/FWGS/hlsdk-portable) holds the live port of the StargateTC game logic (server `dlls/` and client `cl_dll/` code) targeting both GoldSrc/Steam Half-Life and Xash3D.

## How this repo fits in

- **This repo (`jr551/stargatetc-hlsdk-portable`)** — the live canonical game-logic port. Work here.
- [`jr551/stargatetc-hlsdk-xash3d`](https://github.com/jr551/stargatetc-hlsdk-xash3d) — Xash3D-oriented mirror of the same SDK port (default branch `sgtc-port`).
- [`jr551/stargatetc-portable`](https://github.com/jr551/stargatetc-portable) — the runnable game package: Xash3D engine binaries + `stargatetc/` mod dir + bots, with both SDK trees included as git submodules, plus the test gates. Build the DLLs here, package and test there.

The historical Source-engine and reverse-engineering experiments live in the `stargatetc-archive-*` repos and are no longer maintained.

## Building

Clone recursively (the `vgui` submodule is needed):

```
git clone --recursive https://github.com/jr551/stargatetc-hlsdk-portable
```

Configure and build with cmake (produces `hl.dll`/`hl.so` in `build/dlls` and `client.dll`/`client.so` in `build/cl_dll`):

```
cmake -DCMAKE_BUILD_TYPE=Release -B build -S .
cmake --build build
```

On Windows, force a 32-bit build with `-A Win32`:

```
cmake -A Win32 -B build
cmake --build build --config Release
```

To install straight into a mod directory, set `GAMEDIR` (the gamedir name, e.g. `stargatetc`) and `CMAKE_INSTALL_PREFIX` (your Half-Life or Xash3D install root) at configure time, then build the `install` target:

```
cmake -S . -B build -DGAMEDIR=stargatetc -DCMAKE_INSTALL_PREFIX="/path/to/Xash3D"
cmake --build build --target install
```

Note the engine is 32-bit: on Linux you'll need multilib compilers and `libsdl2-dev:i386` (or equivalent). For packaging, bots, and end-to-end test gates, see [jr551/stargatetc-portable](https://github.com/jr551/stargatetc-portable).

## Upstream

Upstream is [FWGS/hlsdk-portable](https://github.com/FWGS/hlsdk-portable) — a maintained, portable Half-Life SDK for GoldSource and Xash3D with numerous bugfixes. The full upstream changelog, per-platform build guides (Steam Runtime chroot, Android, macOS, etc.), mod-branch support notes, and the complete list of bugfix macros and server/client cvars live in the [upstream README](https://github.com/FWGS/hlsdk-portable/blob/master/README.md) and [wiki](https://github.com/FWGS/hlsdk-portable/wiki).
