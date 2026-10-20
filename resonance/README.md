# Resonance

A folder-first, fully offline desktop music player built with C++20, Qt 6 and
QML. Linux is the primary target; the core is portable to Windows and macOS.

**Status: M2 — folder playback, queue, gapless advance and metadata.**

The app opens a folder, scans it recursively, reads tags and cover art, and
plays a gapless natural-order queue with shuffle, repeat, next/previous track,
folder navigation and session restore. EQ, time-stretch, reverb, waveform and
MPRIS are still ahead.

## What works

### Playback and folder queue

- Open a folder with the dialog, drag-and-drop a folder/files anywhere, or pass
  a folder on the command line.
- Recursive background scan; natural sort (`2 - Song` before `10 - Song`).
- Folder tree in the sidebar with expand/collapse, recursive track counts,
  current-folder highlight and direct folder playback.
- Clicking a folder plays its recursive contents immediately; clicking a track
  plays the folder queue from that point.
- Gapless playback across track boundaries. The decoder prefetches the next
  file and pushes a boundary marker; the audio callback switches the audible
  track at the exact frame without stopping the stream.
- Next/previous track, previous restarting the current track at the queue start.
- Shuffle within the selected folder, and repeat modes **Off / Track / Folder**.
- Next/previous folder (`Ctrl+Alt+←/→`) using the natural folder-tree order.
- Play/pause, stop, seek, smoothed volume, queue position (`1 / 12`).

### Tags and artwork

- TagLib 2.x metadata: title, artist, album, track number, year and duration.
- Filename fallback when tags are missing; tracks remain usable before the
  background metadata batches finish.
- Embedded cover art (MP3/FLAC/MP4/OGG via TagLib complex properties) with a
  sidecar fallback (`cover.jpg`, `folder.png`, `front.jpg`, etc.).
- Covers are resized and cached in the standard cache location; the transport
  and playback-tools panel show the current track's artwork.
- Metadata is read in 128-track background batches; the UI never waits for a
  10k-file tag scan.

### Session state

- Last folder, selected folder, last track and position are saved every 5 s and
  on normal quit.
- On launch the session is restored **paused** at the saved track and position.
- Theme, accent, volume, shuffle/repeat modes and right-panel state persist via
  `QSettings`.

### UI and shortcuts

- Custom dark/light theme, six accent colors, rounded surfaces, custom controls.
- Collapsible playback-tools panel with now-playing details.
- Keyboard: `Space`, `Ctrl+O`, `Ctrl+←/→` seek, `Ctrl+↑/↓` volume,
  `Alt+←/→` tracks, `Ctrl+Alt+←/→` folders, `S` shuffle, `R` repeat,
  `Ctrl+T` tools, `F5` rescan, `Ctrl+Q` quit.

## Known M2 limitations

- Search/filter over the current folder is not implemented yet (M6).
- Cover art is shown for the current track, not as a thumbnail in every row.
- The filesystem watch and true incremental diff are not implemented; `F5`
  performs a background path-only rescan and reuses cached tags/covers.
- Gapless uses the current engine only. The DSP chain (EQ/reverb/SoundTouch)
  will need a continuous stream across boundaries in M4/M5.
- TagLib **2.0+** is required for metadata. Without it the app still builds and
  plays files using filename titles and no durations/covers (CMake warns).
- No playlists/M3U (M3), EQ (M4), speed/pitch/reverb/A–B loop (M5), waveform,
  spectrum, mini player, MPRIS or media keys (M6).
- The native folder dialog logs an xdg-desktop-portal app-ID warning until a
  `.desktop` file is installed; harmless.

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

`taglib-devel` on current Fedora is TagLib 2.x. SoundTouch/FFmpeg development
packages are only needed for M5/later.

### Ubuntu

Ubuntu 24.04 ships Qt 6.4, which is older than the required Qt 6.5, and its
`libtag1-dev` is TagLib 1.x. Use a newer Ubuntu release or install Qt 6.5+ and
TagLib 2.x locally, then point CMake at them:

```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_PREFIX_PATH="$HOME/Qt/6.8.0/gcc_64" \
    -DRESONANCE_TAGLIB_ROOT=/path/to/taglib-2.x/prefix
```

Otherwise install the Qt 6 QML modules and `libtag1-dev`; the player will build
without tag metadata and print a CMake warning.

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

If the distro `-devel` package is unavailable, CMake automatically uses a local
TagLib tree at `build/local-deps/usr` when it exists. A normal install can also
be selected explicitly:

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

- `test_ring_buffer` — SPSC ordering, wraparound, partial transfers and a
  concurrent producer/consumer stress test.
- `test_biquad` — flat response, boost/cut, stability.
- `test_audio_engine` — play/pause/seek/stop, gapless queue advance, paused
  restore position and user next/previous navigation on a null device.
- `test_play_queue` — natural/repeat/shuffle ordering, user next vs. repeat
  track, and strict queue semantics.
- `test_natural_sort` — numeric runs, paths and tie-breaking.
- `test_folder_scanner` — recursion, natural order, hidden/unsupported files.
- `test_folder_tree` — nested tree construction, track counts, expansion,
  current folder and DFS folder navigation.
- `test_metadata` — filename fallback, TagLib duration and sidecar cover cache.

`RESONANCE_NULL_AUDIO=1` selects miniaudio's null backend for headless work.
The audio-engine test sets it automatically.

## Architecture

```
src/core/        Pure C++20 engine and DSP (no Qt GUI dependency)
  audio/         AudioEngine, decoders, SPSC sample ring, track boundary queue
  model/         PlayQueue (natural/repeat/shuffle order)
  dsp/           Biquad (EQ building block)
src/library/     Scanner, TagLib metadata, metadata batches, cover cache,
                 natural sort, supported formats
src/ui/
  controllers/   PlayerController, LibraryController, TrackListModel,
                 FolderTreeModel, AppSettings
  qml/           Main window, custom controls, theme, folder tree, panels
resources/       In-house SVG icons
tests/           Qt Test suites
third_party/     Vendored miniaudio and stb_vorbis
```

Audio path:

```
decoder thread → 5 s SPSC ring + boundary markers → audio callback
                                                     → gain/limiter → device
```

The decoder prefetches the next queue item and pushes a `TrackBoundary` at the
end of the current track. The callback advances the audible queue index,
duration and position when it consumes that frame, so metadata changes exactly
when the next track becomes audible. A seek flushes the ring and boundary queue
through a generation handshake; the real-time callback never locks or allocates.

## Roadmap

- **M3** — playlists, drag reordering, queue/play-next, JSON persistence,
  M3U/M3U8 and PLS import/export.
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
