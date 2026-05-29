# CPTR480-Trombone

Rudimentary electronic trombone implemented on the WWU CPTR480 2026 board.

## Pico C/C++ Skeleton

This repository now includes a minimal Raspberry Pi Pico C/C++ CMake skeleton.

### Requirements

- CMake 3.13+
- Ninja
- A local `pico-sdk` checkout
- Environment variable `PICO_SDK_PATH` set to that checkout

### Build

```bash
cmake --preset default
cmake --build --preset default
```

The Pico application target is defined in `src/CMakeLists.txt`.
