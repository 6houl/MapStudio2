# Endless Map Studio

Endless Map Studio is a native Windows C++20 editor for Endless Online EMF maps. It provides the classic floating-panel editing interface, EGF graphics loading, entity and flag editing, compatible EMF persistence, and Map Together live collaboration.

## Quick start

Requirements are Windows, CMake 3.20 or newer, Git, and a Visual Studio C++ toolchain. The first configure needs network access so CMake can fetch EOLib C 0.6.1.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The application is written to `build/Release/EndlessMapStudio.exe`. Run it from there; it locates the repository's `gfx/` and `data/maps/` directories relative to the executable.

The editor requires compatible `gfxNNN.egf` files in `gfx/` to render EO graphics. The files under `data/maps/` are the local map corpus used by the editor and compatibility tests.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Building](docs/BUILDING.md)
- [Testing](docs/TESTING.md)
- [EMF/map compatibility](docs/MAP_FORMAT.md)
- [Map Together V1](docs/MAP_TOGETHER_V1.md)

The consolidation test baseline is 205 automated tests. See the testing guide for the maintained GUI acceptance harness and current invocation.
