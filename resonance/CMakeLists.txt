cmake_minimum_required(VERSION 3.21)

project(resonance VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Concurrent
)

# Optional local TagLib prefix. Useful when the distro -devel package is not
# installed; normal builds can leave this empty. A development checkout that
# has extracted the RPM headers/libs into build/local-deps gets them picked up
# automatically.
set(RESONANCE_TAGLIB_ROOT "" CACHE PATH
    "Optional TagLib installation prefix (for example /usr or a local sysroot)")
if(NOT RESONANCE_TAGLIB_ROOT)
    set(_resonance_local_taglib "${CMAKE_SOURCE_DIR}/build/local-deps/usr")
    if(EXISTS "${_resonance_local_taglib}/lib64/cmake/taglib")
        set(RESONANCE_TAGLIB_ROOT "${_resonance_local_taglib}"
            CACHE PATH "Optional TagLib installation prefix" FORCE)
    endif()
endif()
if(RESONANCE_TAGLIB_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${RESONANCE_TAGLIB_ROOT}")
endif()
find_package(TagLib 2.0 CONFIG QUIET)

qt_standard_project_setup(REQUIRES 6.5)
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

include(CTest)
include(GNUInstallDirs)

option(RESONANCE_BUILD_TESTS "Build the Resonance unit tests" ON)

if(MSVC)
    set(RESONANCE_WARNING_FLAGS /W4)
else()
    set(RESONANCE_WARNING_FLAGS -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Core: pure C++ audio engine and DSP
# ---------------------------------------------------------------------------

add_library(resonance_core STATIC
    src/core/audio/audio_engine.cpp
    src/core/audio/audio_engine.h
    src/core/audio/boundary_queue.h
    src/core/audio/decoder.h
    src/core/audio/miniaudio_decoder.cpp
    src/core/audio/miniaudio_decoder.h
    src/core/audio/spsc_ring_buffer.h
    src/core/dsp/biquad.cpp
    src/core/dsp/biquad.h
    src/core/model/play_queue.cpp
    src/core/model/play_queue.h
    third_party/miniaudio/miniaudio.c
)

target_include_directories(resonance_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb
)

target_compile_options(resonance_core PRIVATE ${RESONANCE_WARNING_FLAGS})
if(NOT MSVC)
    target_compile_options(resonance_core PRIVATE $<$<COMPILE_LANGUAGE:C>:-w>)
endif()

find_package(Threads REQUIRED)
target_link_libraries(resonance_core
    PUBLIC
        Threads::Threads
        ${CMAKE_DL_LIBS}
)

# ---------------------------------------------------------------------------
# Library: scanner, playlists, persistence, metadata
# ---------------------------------------------------------------------------

add_library(resonance_library STATIC
    src/library/audio_formats.h
    src/library/cover_art_cache.cpp
    src/library/cover_art_cache.h
    src/library/folder_scanner.cpp
    src/library/folder_scanner.h
    src/library/metadata_loader.cpp
    src/library/metadata_loader.h
    src/library/metadata_reader.cpp
    src/library/metadata_reader.h
    src/library/natural_sort.cpp
    src/library/natural_sort.h
    src/library/track_metadata.h
)

target_include_directories(resonance_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(resonance_library PRIVATE ${RESONANCE_WARNING_FLAGS})
target_link_libraries(resonance_library
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Concurrent
)

if(TagLib_FOUND)
    if(TARGET TagLib::tag)
        target_link_libraries(resonance_library PUBLIC TagLib::tag)
    elseif(TARGET TagLib::TagLib)
        target_link_libraries(resonance_library PUBLIC TagLib::TagLib)
    endif()
    target_compile_definitions(resonance_library PUBLIC RESONANCE_HAS_TAGLIB=1)
    message(STATUS "TagLib found: metadata and cover extraction enabled")
else()
    message(WARNING
        "TagLib >= 2.0 not found. Building without tag/cover support; "
        "install taglib-devel (2.x) or pass -DRESONANCE_TAGLIB_ROOT=<prefix>.")
endif()

# ---------------------------------------------------------------------------
# Application and QML module
# ---------------------------------------------------------------------------

if(MSVC)
    set_source_files_properties(third_party/miniaudio/miniaudio.c PROPERTIES COMPILE_OPTIONS "/w")
else()
    set_source_files_properties(third_party/miniaudio/miniaudio.c PROPERTIES COMPILE_OPTIONS "-w")
endif()

set_source_files_properties(
    src/ui/qml/theme/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_executable(resonance
    src/main.cpp
    src/resources/resources.qrc
    src/ui/controllers/app_settings.cpp
    src/ui/controllers/app_settings.h
    src/ui/controllers/library_controller.cpp
    src/ui/controllers/library_controller.h
    src/ui/controllers/player_controller.cpp
    src/ui/controllers/player_controller.h
)

qt_add_qml_module(resonance
    URI Resonance
    VERSION 1.0
    QML_FILES
        src/ui/qml/Main.qml
        src/ui/qml/theme/Theme.qml
        src/ui/qml/controls/Icon.qml
        src/ui/qml/controls/IconButton.qml
        src/ui/qml/controls/ResonanceButton.qml
        src/ui/qml/controls/ResonanceSlider.qml
        src/ui/qml/controls/SectionLabel.qml
        src/ui/qml/components/HeaderBar.qml
        src/ui/qml/components/FolderTreePane.qml
        src/ui/qml/components/NowPlayingPanel.qml
        src/ui/qml/components/Sidebar.qml
        src/ui/qml/components/TrackListPane.qml
        src/ui/qml/components/TransportBar.qml
    SOURCES
        src/ui/controllers/track_list_model.cpp
        src/ui/controllers/track_list_model.h
        src/ui/controllers/folder_tree_model.cpp
        src/ui/controllers/folder_tree_model.h
)

target_include_directories(resonance PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui/controllers
)
target_compile_options(resonance PRIVATE ${RESONANCE_WARNING_FLAGS})
target_link_libraries(resonance
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Qml
        Qt6::Concurrent
        resonance_core
        resonance_library
)

set_target_properties(resonance PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS resonance
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

if(BUILD_TESTING AND RESONANCE_BUILD_TESTS)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()
