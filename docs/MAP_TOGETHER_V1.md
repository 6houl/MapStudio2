# Map Together V1 developer notes

## Accepted V1 baseline

Map Together V1 was accepted before repository consolidation with a passing Release build and 205/205 automated tests. The definitive two-process run converged at revision 340 with host, client, and saved-map fingerprint `dae607fb`, zero pending requests on both peers, valid backups and metadata, and a 30-second continuous-edit soak. Generated screenshots, maps, and logs are reproducible through the maintained acceptance harness and are not permanent source files.

The clean post-consolidation build was accepted again at revision 339 with host/client fingerprint `e26a2dfd`, zero pending requests, visible collaboration-window verification, semantic disk convergence, and a 30.062-second continuous-edit soak.

## Architecture

Map Together uses a direct host/client session over the version 5 collaboration protocol. The host owns the authoritative map revision and validates every requested mutation. Editors submit operations optimistically and reconcile them against the host broadcast; Viewers can publish presence but cannot mutate map state. A revision gap disconnects the client rather than applying ambiguous state.

Graphics, flags, map-wide operations, and committed entities are synchronized as bounded operations. Signs, NPC collections, items, and warps use per-tile, per-scope generations so a stale form cannot overwrite a newer committed entity. Presence is transient and does not change the map revision or dirty state. Participant role and password administration remain host-only.

## Persistence

Normal disconnected saves and host-authoritative collaborative saves use the same production pipeline. A connected Editor or Viewer is rejected before any file chooser, comment dialog, backup, disk write, SaveEvent, or revision effect.

Before hosting starts, the editor creates one `session-start` EMF recovery file. If the map has a source file, that on-disk state is copied; an unsaved document is serialized through the verified EMF writer without changing its path or dirty state. Hosting is not exposed if this backup fails.

Before overwriting an existing map, its exact previous bytes are written to `maps\backups\`. The new EMF is written to a sibling temporary file, flushed and closed, then atomically replaced. A required backup failure or EMF write/replace failure leaves the previous authoritative file intact. Only a successful EMF replacement updates the current path and clears dirty state.

The EO EMF schema used by the project has no verified map-comment field. Comments are therefore never appended to EMF. The editor writes versioned UTF-8 JSON under `maps\metadata\<map filename>.json` with the map filename and a `lastSave` object containing timestamp, exact optional comment, file size, collaborative status, and author display name. Metadata also uses temporary-file replacement. Metadata failure is reported but does not roll back a successfully saved EMF.

After a collaborative host EMF save succeeds, the host emits a `SaveEvent` containing only author ID, current revision, and timestamp. Saving does not increment the revision. Clients show a quiet “Map saved by …” notice and never show the host's result dialog.

## Security

Authentication uses the session password verifier already defined by the collaboration protocol. Passwords are not included in snapshots, operation traffic, SaveEvents, metadata, or acceptance state. Metadata contains no network address, absolute path, or participant collection.

The host enforces Viewer read-only status; disabling editor controls is not the security boundary. Protocol payloads, display names, comments, entity collections, and operation batches are bounded and validated before application. Save comments are valid UTF-8 and limited to 4096 bytes. Backup names are sanitized and all persistence paths are derived from the selected map directory or the executable-aware project maps directory for unsaved session recovery.

## Recovery

Recovery EMFs are ordinary compatible map files in the `backups` directory beside the map directory. Names include the sanitized map identity, purpose (`session-start` or `pre-save`), local date/time with milliseconds, and a collision suffix when required.

To recover manually:

1. Close the editor or disconnect the active session.
2. Locate the relevant `.emf` in the map directory's `backups` folder.
3. Copy it to a new filename in the map directory; do not overwrite the current file until it has been inspected.
4. Open the copy in Endless Map Studio and verify its map contents.
5. Use Save As to promote the verified recovery copy if needed.

The metadata sidecar is supplemental. A valid EMF remains authoritative and openable if metadata is absent or damaged.
