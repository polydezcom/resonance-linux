# Third-party software

Resonance itself is licensed under GPL-3.0-or-later (see `LICENSE`). This file
lists every third-party component used by the current milestone, plus
dependencies planned for later milestones.

## Used in M2

| Component | Version | License | How it is used |
|---|---|---|---|
| Qt 6 | 6.5+ | LGPL-3.0 (or GPL-3.0 / commercial) | Core, Gui, Qml, Quick, Quick Controls 2, Concurrent, Test |
| miniaudio | 0.11.22 | MIT-0 **or** public domain | Audio device I/O, WAV/FLAC/MP3 decoding |
| stb_vorbis | 1.22 | Public domain **or** MIT | Ogg Vorbis decoding (compiled with miniaudio) |
| TagLib | 2.0+ (2.3 tested) | LGPL-2.1 **or** MPL-2.0 | Tags, durations, embedded cover art |

### Qt 6

Qt is linked dynamically. The application does not modify Qt and users can
replace the Qt libraries with compatible builds. Qt's LGPL-3.0 terms apply; see
<https://www.qt.io/licensing/open-source-lgpl-obligations>. The bundled Qt Quick
Controls **Basic** style is part of Qt and is customized with application QML,
not patched.

### miniaudio

Vendored at `third_party/miniaudio/miniaudio.h`; the implementation is compiled
from `third_party/miniaudio/miniaudio.c`. miniaudio is available under either
MIT-0 or the public domain. Its header contains the full license text. The
version is pinned at 0.11.22.

miniaudio's decoder code is derived from the public-domain/MIT-0 dr_libs family
(`dr_wav`, `dr_flac`, `dr_mp3`).

### stb_vorbis

Vendored at `third_party/stb/stb_vorbis.c`, version 1.22, public domain or MIT.
The full dual-license text is at the end of that file. It is compiled into the
same translation unit as miniaudio so miniaudio exposes its Vorbis decoder.

### TagLib

TagLib 2.x is linked dynamically. It is dual-licensed LGPL-2.1 / MPL-2.0.
Resonance uses the public C++ API without patches. The app can be built without
TagLib; in that case metadata, durations and embedded covers are disabled and
only filename fallbacks are available. M2 requires the TagLib 2.x
`complexProperties()` / `Variant` API for cover extraction.

Development note: the local TagLib headers and CMake config in
`build/local-deps/` (if present) come from the distribution's `taglib-devel`
RPM and are gitignored. Production builds should use the distro package or a
normal TagLib prefix via `-DRESONANCE_TAGLIB_ROOT`.

### Icons and artwork

All SVGs in `src/resources/icons/` were created for this project and are
covered by the project's GPL-3.0-or-later license.

## Planned for later milestones

| Component | Planned milestone | License | Notes |
|---|---|---|---|
| SoundTouch 2.4 | M5 | LGPL-2.1+ | Dynamic linking planned; tempo/pitch shifting |
| FFmpeg (libavcodec/libavformat/libavutil/libswresample) | later | LGPL-2.1+ when built without `--enable-gpl` | Optional, dynamically linked; AAC/M4A/Opus/WMA support |

### FFmpeg distribution note

FFmpeg's license depends on how the library was built. A build with GPL or
non-free components is not compatible with the intended LGPL distribution
story unless the whole application is distributed under matching terms. Release
packages should use an FFmpeg build configured without `--enable-gpl` and
without `--enable-nonfree`, and should document the exact build configuration.
The app keeps FFmpeg optional so a pure-LGPL build remains possible.

### SoundTouch distribution note

SoundTouch is LGPL-2.1+. It will be linked dynamically so users can replace it.
If a statically linked build is ever offered, the relinkable-object requirements
of the LGPL must be met.

## Build tools

CMake, Ninja and the C++ compiler are build-time tools and are not distributed
as part of the application. Qt Test is part of Qt and is used only for the test
binaries.
