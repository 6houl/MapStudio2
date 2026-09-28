# Testing

## Automated suite

Configure and build first, then run the complete suite:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

At the September 2026 consolidation checkpoint the expected baseline is 205 passing tests. The count is a checkpoint, not a permanent contract; new coverage may increase it.

The suite covers:

- EMF header parsing, edit/save/reload, and the map corpus under `data/maps/`
- EGF bitmap and bank enumeration
- model operations, map resize, layers, flags, warps, and entities
- editor camera and independent editor-state behavior
- collaboration protocol, authentication, networking, synchronized edits, presence, roles, and administration
- save authorization, backups, atomic replacement, metadata, comments, failure injection, and result presentation

Maintained test source is under `tests/`. Generated test executables and per-test EMF output stay under `build/`.

## Map Together GUI acceptance

Run the maintained two-process harness from the repository root in an interactive Windows desktop session:

```powershell
powershell -ExecutionPolicy Bypass -File tests/acceptance/run_map_together_acceptance.ps1
```

The harness launches the Release executable from its final build location and verifies host/client connection, synchronized graphics and flags, entity/map operations, role enforcement, off-screen presence, password administration, host-authoritative save, session and pre-save backups, metadata, Save As and failure paths, drained pending edits, and semantic host/client/disk convergence. Generated maps, JSON, logs, and screenshots go to a timestamped directory under `build/acceptance/`.

An optional explicit output directory may be passed with `-OutputDirectory`.

## Manual UI stress tool

`tests/manual/run_ui_stress.ps1` repeatedly moves and resizes panels and exercises camera/tool inputs while monitoring GDI and USER handles. It is intentionally not part of CTest because it requires an interactive desktop and runs for about five minutes.

There is no separate test-only binary fixture directory at this checkpoint. The maintained EMF corpus remains under `data/maps/` because it is also valid editor/runtime data; files of uncertain user provenance were not reclassified as disposable fixtures.
