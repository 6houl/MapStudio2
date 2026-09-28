# Architecture

Endless Map Studio is a native C++20 Windows desktop application built directly on Win32, common controls, and GDI. CMake fetches EOLib C 0.6.1 for EO data support. There is no game-server connection or generated packet layer.

## Source layout

- `src/main.cpp` owns the application entry point, Win32 message routing, the `MapDocument` model, EMF parsing/serialization integration, editing commands, and the panel controllers. This remains intentionally monolithic until a focused refactor can move responsibilities without mixing architectural work with maintenance.
- `src/editor/` contains the camera and the durable editor-selection/tool state used by the viewer and tests.
- `src/ui/` contains reusable classic Win32/GDI drawing helpers.
- `src/collaboration/` contains the protocol codec, authoritative host/client session, synchronized edit model, and Map Together window.
- `src/persistence/` contains the transactional save, backup, metadata-sidecar, and result-presentation workflow.

## Editor model and UI

`MapDocument` is the in-memory authoritative map. It contains map metadata, dimensions, the base/fill tile, nine graphic layers per tile, tile specs and warps, and the NPC, item, sign, and legacy door-key collections. Undo/redo stores document snapshots. The viewer renders this model using EGF bitmap resources; editing updates the model and invalidates the affected panels.

`EditorCamera` converts between isometric map coordinates and viewport coordinates and owns zoom and continuous pan state. Floating child panels expose the Viewer, Graphics, Layers, Map Properties, Flags, Toolset, and Entities surfaces. `EditorState` keeps tool, brush, selected graphics, flag, and layer choices independent so switching panels does not silently alter another editing domain.

## EMF and EGF

EMF bytes are parsed and serialized through the verified EO-compatible reader/writer code in `main.cpp` with EOLib data primitives. Round-trip and full-corpus tests protect compatibility. EGF banks are loaded as Windows modules from `gfx/gfxNNN.egf`; bitmaps are cached by bank and resource ID.

At runtime, asset discovery starts at the executable directory and checks parent directories. With the standard build layout this resolves the repository's `gfx/` and `data/maps/` directories without relying on the shell's current directory.

## Collaboration

Map Together uses protocol version 5 over direct TCP. The host owns the map revision, validates edit requests, broadcasts accepted operations, and controls participant roles and the optional session password. Clients reconcile optimistic edits against authoritative broadcasts. Presence is transient and does not modify the map revision or dirty state. See [MAP_TOGETHER_V1.md](MAP_TOGETHER_V1.md) for persistence, recovery, and security behavior.

## Persistence

Disconnected saves and host-authoritative collaborative saves share the `save_workflow` pipeline. Existing maps receive a pre-save backup, new bytes are written to a sibling temporary file, and replacement is atomic. Save comments and save-result metadata live in JSON sidecars rather than modifying EMF. Session-start backups provide recovery before hosting begins.
