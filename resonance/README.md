# Resonance

A folder-first, fully offline desktop music player built with C++20, Qt 6 and
QML. Linux is the primary target; the core is portable to Windows and macOS.

**Status: M3 — folder queue, gapless playback, metadata and playlists.**

The player now supports recursive folder playback, a gapless queue, tags and
cover art, persistent playlists with M3U/M3U8/PLS import/export, drag reordering,
"play next", and session restore. EQ, time-stretch, reverb, waveform and MPRIS
are still ahead.

## What works

### Folder playback and queue

- Open a folder with the dialog, drag-and-drop, or pass a folder on the command
  line.
- Recursive background scan, natural sort (`2 - Song` before `10 - Song`).
- Sidebar folder tree with nested folders, recursive track counts,
  expand/collapse, current-folder highlight and direct folder playback.
- Clicking a folder plays its recursive contents; clicking/double-clicking a
  track plays the folder queue from that point.
- Gapless auto-advance: the decoder prefetches the next file and pushes a
  frame-accurate boundary; the audio callback switches the audible track at the
  exact frame.
- Next/previous track, previous restarting the current track when already at the
  queue start, shuffle within the selected folder, and repeat
  **Off / Track / All**.
- Next/previous folder (`Ctrl+Alt+←/→`) using natural folder-tree order.
- Play/pause, stop, seek, smoothed volume, queue position (`1 / 12`).

### Playlists

- Multiple playlists in the sidebar, with current-playlist highlight.
- Create, rename and delete playlists; delete asks for confirmation.
- Add an entire folder to the selected playlist, remove tracks, and drag-reorder
  tracks with the grip handle.
- Drop files or folders anywhere while a playlist is active to add them to that
  playlist.
- "Play next" on a track inserts it immediately after the current track in the
  playback queue.
- Import and export **M3U/M3U8** and **PLS**. Relative paths are resolved against
  the playlist file and exports are written relative to it.
- Playlists auto-save to versioned JSON in the standard application-data
  location using atomic `QSaveFile` writes.
- Playlist playback uses the same gapless queue; repeat **All** wraps the
  playlist.
- Session restore remembers whether the last session was a folder or a
  playlist.

### Tags and artwork

- TagLib 2.x metadata: title, artist, album, track number, year, duration.
- Filename fallback when tags are missing; tracks stay usable before metadata
  batches finish.
- Embedded covers plus sidecar fallback (`cover.jpg`, `folder.png`, `front.jpg`,
  etc.), resized and cached in the standard cache location.
- Metadata is read in 128-track background batches.

### Session state

- Last folder, selected folder, track, playlist, mode and position are saved
  every 5 seconds and on normal quit.
- On launch the session is restored **paused** at the saved track/position.
- Theme, accent, volume, shuffle/repeat modes and the right-panel state persist
  via `QSettings`.

### UI and shortcuts

- Custom dark/light theme, six accent colors, rounded surfaces, custom controls.
- Collapsible playback-tools panel with now-playing details.
- Keyboard: `Space`, `Ctrl+O`, `Ctrl+←/→` seek, `Ctrl+↑/↓` volume,
  `Alt+←/→` tracks, `Ctrl+Alt+←/→` folders, `S` shuffle, `R` repeat,
  `Ctrl+T` tools, `F5` refresh, `Ctrl+Q` quit.

## Known M3 limitations

- Search/filter over the current playlist or folder is not implemented yet (M6).
- Playlist tracks are stored as paths. Missing files remain in the list and
  fail only when played; there is no missing-file badge or automatic cleanup.
- Cover art is shown for the current track, not as a thumbnail in every row.
- The filesystem watch and true incremental diff are not implemented; `F5`
  does a fast background path-only rescan and reuses cached tags/covers.
- "Play next" restarts the current stream at the current position so the
  inserted item can be queued correctly; this can cause a very short audible
  interruption. A true mid-stream queue insertion is planned for a later pass.
- TagLib **2.0+** is required for metadata. Without it the app builds and plays
  using filename titles and no durations/covers (CMake warns).
- No EQ (M4), speed/pitch/reverb/A–B loop (M5), waveform, spectrum, mini player,
  MPRIS, media keys or search (M6).

## Build

Requirements:

- CMake 3.21+
- C++20 compiler (GCC 11+, Clang 14+, MSVC 19.3+)
- Qt **6.5 or newer** (Core, Gui, Qml, Quick, QuickControls2, Concurrent, Test)
- TagLib **2.0+** for tags and covers (optional at build time)
- Ninja (recommended)
- On Linux: ALSA/PulseAudio/PipeWire output

The project vendors miniaudio 0.11.22 and stb_vorbis, so no network access is
needed to build or run the player.

### Fedora

```bash
sudo dnf install cmake ninja-build gcc-c++ \
    qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg \
    taglib-devel
```

### Ubuntu

Ubuntu 24.04 ships Qt 6.4, older than the required Qt 6.5, and its
`libtag1-dev` is TagLib 1.x. Use a newer Ubuntu release or install Qt 6.5+ and
TagLib 2.x locally:

```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.0/gcc_64" \
    -DRESONANCE_TAGLIB_ROOT=/path/to/taglib-2.x/prefix
```

The player builds without TagLib and prints a CMake warning if TagLib 2.x is
missing.

```bash
sudo apt install build-essential cmake ninja-build \
    qt6-base-dev qt6-declarative-dev \
    qml6-module-qtquick-controls qml6-module-qtquick-layouts \
    qml6-module-qtquick-dialogs qml6-module-qtquick-effects \
    qml6-module-qtquick-window qml6-module-qtqml-workerscript \
    libqt6svg6-dev libgl1-mesa-dev
```

### Arch Linux

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-svg taglib
```

### Offline TagLib sysroot (development)

CMake automatically uses a local TagLib tree at `build/local-deps/usr` when it
exists. An explicit prefix can also be selected:

```bash
cmake -S . -B build -G Ninja -DRESONANCE_TAGLIB_ROOT=/usr
```

### Build and run

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
./build/resonance
```

Start directly in a folder:

```bash
./build/resonance /path/to/music
```

Disable tests with `-DRESONANCE_BUILD_TESTS=OFF`.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

- `test_ring_buffer` — SPSC ordering, wraparound and concurrent stress.
- `test_biquad` — flat response, boost/cut and stability.
- `test_audio_engine` — play/pause/seek/stop, gapless advance, paused restore
  position and user next/previous navigation on a null device.
- `test_play_queue` — natural/repeat/shuffle ordering and user navigation.
- `test_natural_sort` — numeric runs, paths and tie-breaking.
- `test_folder_scanner` — recursion, natural order, hidden/unsupported files.
- `test_folder_tree` — nested construction, counts, expansion, current folder
  and DFS folder navigation.
- `test_metadata` — filename fallback, TagLib duration and sidecar covers.
- `test_playlist_manager` — JSON round-trip, CRUD, reordering, unique names and
  malformed files.
- `test_playlist_io` — M3U8/PLS round-trips, relative paths, comments and errors.
- `test_player_controller` — queue insertion ("play next") and controller-level
  next navigation against the null device.

`RESONANCE_NULL_AUDIO=1` selects miniaudio's null backend for headless work.
The audio-engine test sets it automatically.

## Architecture

```
src/core/        Pure C++20 engine and DSP (no Qt GUI dependency)
  audio/         AudioEngine, decoders, SPSC sample ring, boundary queue
  model/         PlayQueue (natural/repeat/shuffle order)
  dsp/           Biquad (EQ building block)
src/library/     Scanner, TagLib metadata, metadata batches, cover cache,
                 natural sort, playlist manager, M3U/PLS import/export
src/ui/
  controllers/   PlayerController, LibraryController, PlaylistController,
                 TrackListModel, FolderTreeModel, PlaylistListModel, AppSettings
  qml/           Main window, custom controls, theme, folder tree, playlists
resources/       In-house SVG icons
tests/           Qt Test suites
third_party/     Vendored miniaudio and stb_vorbis
```

Audio path:

```
decoder thread → 5 s SPSC ring + boundary markers → audio callback
                                                     → gain/limiter → device
```

The decoder prefetches the next track and pushes a `TrackBoundary` at the end
of the current one. The callback advances the audible queue index, duration and
position when it consumes that frame. Seeking and queue jumps flush the ring and
boundary queue through a generation handshake; the real-time callback never
locks or allocates.

Playlist JSON lives in `QStandardPaths::AppDataLocation/playlists.json`;
settings live in `QSettings` (native INI on Linux).

## Roadmap

- **M4** — 10-band graphic EQ, preamp, built-in and user presets.
- **M5** — SoundTouch tempo/pitch, tape mode, Freeverb reverb, A–B loop,
  per-track settings.
- **M6** — waveform seek bar, spectrum, mini player, always-on-top, MPRIS,
  command palette, search/filter and accessibility polish.

## License

Resonance is distributed under the GNU General Public License v3.0 or later;
see `LICENSE`. Third-party components and their licenses are listed in
`THIRD_PARTY.md`.

`importedcontent/` is an unused Qt Creator design-import placeholder.
