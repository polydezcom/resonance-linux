# Resonance

A folder-first, fully offline desktop music player built with C++20, Qt 6 and
QML. Linux is the primary target; the core is portable to Windows and macOS.

**Status: M1 — runnable skeleton.** The app builds, scans a folder recursively,
sorts tracks naturally, plays MP3/FLAC/WAV/Ogg Vorbis through a custom
pull-based audio engine, and has the dark/light shell with transport controls.
Playlists, gapless queue playback, EQ, time-stretch, reverb, waveform and MPRIS
arrive in later milestones.

## What works in M1

- Custom Qt Quick Controls 2 look (Basic style + own controls), dark and light
  themes, six accent colors, animated hover/focus states.
- Open a folder with the dialog, drag-and-drop a folder or files anywhere, or
  pass a folder on the command line.
- Recursive background scan; natural sort (`2 - Song` before `10 - Song`) via
  `QCollator` numeric mode.
- Track list with folder grouping, keyboard navigation (arrows + Enter),
  double-click to play, current-track highlight and error banner.
- Custom audio engine:
  - decode → device-rate float32 stereo → lock-free SPSC ring → real-time
    miniaudio callback;
  - play/pause, stop, seek, smoothed volume;
  - no allocations, locks or Qt calls in the audio callback;
  - null audio backend for deterministic tests (`RESONANCE_NULL_AUDIO=1`).
- Formats: MP3, FLAC, WAV, Ogg Vorbis (miniaudio + stb_vorbis).
- Last folder is remembered and restored paused on next launch; theme, accent,
  volume and right-panel state persist via `QSettings`.
- Shortcuts: `Space` play/pause, `Ctrl+O` open folder, `Ctrl+←/→` seek 5 s,
  `Ctrl+↑/↓` volume, `Ctrl+T` playback tools, `F5` rescan, `Ctrl+Q` quit.
- Right-hand playback-tools panel is the shell for M4/M5; it currently shows
  the roadmap and now-playing details.

### Known M1 limitations

- No queue, gapless advance, next/previous track, shuffle or repeat yet (M2).
- Track rows show `—` for duration; tags/cover art and metadata caching are M2.
- No playlists/M3U yet (M3), no EQ (M4), no speed/pitch/reverb/A–B loop (M5),
  no waveform/spectrum/mini player/MPRIS (M6).
- Seeking a stream whose duration cannot be determined is refused by the
  transport UI. Ogg Vorbis duration is available with the current stb_vorbis
  integration, but unusual streams may still report `0:00`.
- Opening a new file currently joins the old decoder thread before starting;
  this is quick for local files, but will move fully off the UI thread in M2.

## Build

Requirements:

- CMake 3.21+
- C++20 compiler (GCC 11+, Clang 14+, MSVC 19.3+)
- Qt **6.5 or newer** (Core, Gui, Qml, Quick, QuickControls2, Concurrent, Test)
- Ninja (recommended)
- On Linux: a working ALSA/PulseAudio/PipeWire output

The project vendors miniaudio 0.11.22 and stb_vorbis, so no network access is
needed to build the current milestone.

### Fedora

```bash
sudo dnf install cmake ninja-build gcc-c++ \
    qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg
```

Later milestones (optional for M1):

```bash
sudo dnf install taglib-devel soundtouch-devel ffmpeg-devel
```

`ffmpeg-devel` is available from RPM Fusion on Fedora.

### Ubuntu

Ubuntu 24.04 ships Qt 6.4, which is older than the required Qt 6.5. Use a newer
Ubuntu release, or install Qt 6.5+ with the Qt online installer and point
CMake at it with `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.8.0/gcc_64`.

On a release with Qt 6.5+:

```bash
sudo apt install build-essential cmake ninja-build \
    qt6-base-dev qt6-declarative-dev \
    qml6-module-qtquick-controls qml6-module-qtquick-layouts \
    qml6-module-qtquick-dialogs qml6-module-qtquick-effects \
    qml6-module-qtquick-window qml6-module-qtqml-workerscript \
    libqt6svg6-dev libgl1-mesa-dev
```

Later milestones: `libtag1-dev libsoundtouch-dev libavcodec-dev
libavformat-dev libavutil-dev libswresample-dev`.

### Arch Linux

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-svg
```

Later milestones: `taglib soundtouch ffmpeg`.

### Build and run

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
./build/resonance
```

Open a folder from the UI, or start with one directly:

```bash
./build/resonance /path/to/music
```

Disable tests with `-DRESONANCE_BUILD_TESTS=OFF`.

## Tests

```bash
ctest --test-dir build --output-on-failure
```

Current coverage:

- `test_ring_buffer` — SPSC ordering, wraparound, partial transfers and a
  concurrent producer/consumer stress test.
- `test_biquad` — flat/bypass response, boost/cut at the centre frequency and
  stability.
- `test_audio_engine` — full null-device lifecycle: play, duration, position,
  pause, resume, seek, end-of-stream and stop.
- `test_natural_sort` — numeric runs, paths, case-insensitive tie-breaking and
  strict weak ordering.
- `test_folder_scanner` — recursive scan, natural order, hidden/unsupported
  files, missing and empty folders.

For headless development, `RESONANCE_NULL_AUDIO=1` selects miniaudio's null
device. The audio-engine test sets this automatically.

## Architecture

```
src/core/        Pure C++20 engine and DSP (no Qt GUI dependency)
  audio/         AudioEngine, decoder interface, miniaudio decoder,
                 lock-free SPSC ring buffer
  dsp/           Biquad (EQ building block)
src/library/     Scanner, natural sort, supported formats (QtCore/Concurrent)
src/ui/
  controllers/   QML-facing PlayerController, LibraryController, TrackListModel,
                 AppSettings
  qml/           Main window, custom controls, theme, panels
resources/       In-house SVG icons
tests/           Qt Test suites
third_party/     Vendored miniaudio and stb_vorbis
```

Audio path:

```
decoder thread → 4 s SPSC ring → audio callback
                                  → (M5: SoundTouch → EQ → reverb)
                                  → gain/limiter → device
```

The UI polls atomics from a 33 ms timer and emits Qt signals from the UI thread.
The audio callback itself performs no allocation, locking, logging or signal
emission.

## Roadmap

- **M2** — recursive folder playback/queue, natural subfolder order, gapless
  advance, next/previous folder, shuffle/repeat, tag metadata and covers,
  restore track/position, 10k-file responsiveness.
- **M3** — playlists, drag reordering, queue/play-next, JSON persistence,
  M3U/M3U8 and PLS import/export.
- **M4** — 10-band graphic EQ, preamp, built-in and user presets.
- **M5** — SoundTouch tempo/pitch, tape mode, Freeverb reverb, A–B loop,
  per-track settings.
- **M6** — waveform seek bar, spectrum, mini player, always-on-top, MPRIS,
  command palette and accessibility polish.

## License

Resonance is distributed under the GNU General Public License v3.0 or later;
see `LICENSE`. Third-party components and their licenses are listed in
`THIRD_PARTY.md`.

`importedcontent/` is an unused Qt Creator design-import placeholder.
