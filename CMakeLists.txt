cmake_minimum_required(VERSION 3.20)
project(CompactVLMQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# LGPL compliance: dynamic (shared) libraries required — static linking prohibited
set(BUILD_SHARED_LIBS ON)

# Qt path configuration (MSVC 2022 64-bit)
set(Qt6_DIR "C:/Qt/6.11.1/msvc2022_64/lib/cmake/Qt6")
set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")

find_package(Qt6 6.11 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Qml
    Network
    Concurrent
)

qt_standard_project_setup(REQUIRES 6.6)

# Qt 6.10 policy (QTP0004: suppress qmldir warning)
if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

# ────────────────────────────────────────────────────────────
# Source file list
# ────────────────────────────────────────────────────────────
set(APP_SOURCES
    main.cpp
    src/VLCFrameGrabber.cpp
    src/VideoPlayer.cpp
    src/VideoSurface.cpp
    src/VLMEngine.cpp
    src/VLMBridge.cpp
    src/CaptureImageProvider.cpp
    src/ImageUtils.cpp
)

set(APP_HEADERS
    src/VLCFrameGrabber.h
    src/VideoPlayer.h
    src/VideoSurface.h
    src/VLMEngine.h
    src/VLMBridge.h
    src/CaptureImageProvider.h
    src/ImageUtils.h
)

# ────────────────────────────────────────────────────────────
# Executable
# ────────────────────────────────────────────────────────────
qt_add_executable(CompactVLMQt
    WIN32
    ${APP_SOURCES}
    ${APP_HEADERS}
    app.rc
)

# Embed icon as Qt resource (used by app.setWindowIcon)
qt_add_resources(CompactVLMQt "appicon"
    PREFIX "/"
    FILES resources/app.ico
)

# ────────────────────────────────────────────────────────────
# QML module (LGPL compliance: Qt Quick loaded as shared library)
# ────────────────────────────────────────────────────────────

# Register Theme.qml as a QML singleton (used with pragma Singleton)
set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(CompactVLMQt
    URI CompactVLMQt
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/VideoPanel.qml
        qml/ChatPanel.qml
        qml/StyledButton.qml
        qml/Theme.qml
        qml/AboutDialog.qml
    SOURCES
        src/VideoSurface.h  src/VideoSurface.cpp
        src/VLMBridge.h     src/VLMBridge.cpp
)

# ────────────────────────────────────────────────────────────
# Include directories
# ────────────────────────────────────────────────────────────
target_include_directories(CompactVLMQt PRIVATE
    src
    "C:/libVLC/include"
)

# ────────────────────────────────────────────────────────────
# Link external libraries (LGPL: all dynamically linked)
# ────────────────────────────────────────────────────────────

# libVLC (MSVC import libs)
find_library(LIBVLC_LIB
    NAMES libvlc
    PATHS "C:/libVLC/lib"
    NO_DEFAULT_PATH
)
find_library(LIBVLCCORE_LIB
    NAMES libvlccore
    PATHS "C:/libVLC/lib"
    NO_DEFAULT_PATH
)

if(NOT LIBVLC_LIB)
    message(FATAL_ERROR "libvlc.lib not found in C:/libVLC/lib.")
endif()

target_link_libraries(CompactVLMQt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Qml
    Qt6::Network
    Qt6::Concurrent
    ${LIBVLC_LIB}
    ${LIBVLCCORE_LIB}
    winhttp
    shlwapi
    shell32
    ws2_32
)

# ────────────────────────────────────────────────────────────
# Compiler options
# ────────────────────────────────────────────────────────────
target_compile_options(CompactVLMQt PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W3 /wd4100 /wd4101 /DUNICODE /D_UNICODE>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wno-unused-parameter -DUNICODE -D_UNICODE>
)

target_compile_definitions(CompactVLMQt PRIVATE _CRT_SECURE_NO_WARNINGS)

# ────────────────────────────────────────────────────────────
# Copy VLC plugins to executable directory
# ────────────────────────────────────────────────────────────
add_custom_command(TARGET CompactVLMQt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/libVLC/bin/libvlc.dll"
        $<TARGET_FILE_DIR:CompactVLMQt>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/libVLC/bin/libvlccore.dll"
        $<TARGET_FILE_DIR:CompactVLMQt>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "C:/libVLC/bin/plugins"
        "$<TARGET_FILE_DIR:CompactVLMQt>/plugins"
    COMMENT "Copying VLC DLLs and plugins..."
)


# ────────────────────────────────────────────────────────────
# Run windeployqt automatically (Release build)
# ────────────────────────────────────────────────────────────
if(WIN32)
    set(DEPLOY_TOOL "C:/Qt/6.11.1/msvc2022_64/bin/windeployqt6.exe")
    if(EXISTS ${DEPLOY_TOOL})
        add_custom_command(TARGET CompactVLMQt POST_BUILD
            COMMAND ${DEPLOY_TOOL}
                --qmldir "${CMAKE_SOURCE_DIR}/qml"
                --no-translations
                $<TARGET_FILE:CompactVLMQt>
            COMMENT "Running windeployqt..."
        )
    endif()
endif()
