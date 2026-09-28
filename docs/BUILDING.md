# Building

## Prerequisites

- Windows 10 or later
- CMake 3.20 or newer
- Git
- Visual Studio with the Desktop development with C++ workload, or another Windows C/C++ toolchain supported by CMake
- Network access during the first configure, because CMake fetches EOLib C 0.6.1

## Clean Release build

From the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

The verified Visual Studio-generator output is:

```text
build/Release/EndlessMapStudio.exe
```

The executable is accompanied by the fetched EOLib runtime DLL. CMake owns the rest of `build/`; do not copy generated executables back into the source tree.

## Tests

```powershell
ctest --test-dir build -C Release --output-on-failure
```

RTK is optional for developers who have it installed:

```powershell
rtk proxy cmake --build build --config Release
rtk ctest --test-dir build -C Release --output-on-failure
```

RTK is not a project dependency.

## Runtime data

The editor loads EO bitmap resources from `gfx/gfxNNN.egf` and opens EMF files from any user-selected location. The repository's `data/maps/` directory is also discovered as the default local map directory and is used by compatibility tests. These EO data files are external runtime inputs; CMake does not download them.

`data/pub/` is retained as local EO project data but is not currently read by the executable. Do not replace, redistribute, or delete EO data merely as part of a build cleanup.

The normal `build/Release/` layout is significant: executable-relative lookup checks that directory and its parents to find `gfx/` and `data/maps/`. Launching from the final output path is the supported verification path.
