cmake_minimum_required(VERSION 4.2)
project(timeline_scrub_Qt)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(X11 REQUIRED)
find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libswscale
)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET
        gstreamer-1.0
        gstreamer-video-1.0
        gstreamer-base-1.0
        gstreamer-app-1.0
)
add_executable(timeline_scrub_Qt src/main.cpp
        include/timecode_check.h
        include/video_fetch.h
        src/video_implement.c
        src/video_fetch.c
        src/timecode_check.c
        src/video_window.cpp
        include/video_window.h
        src/video_hotkey_actions.c
        include/video_hotkey_actions.h)

target_link_libraries(timeline_scrub_Qt
        Qt::Core
        Qt::Gui
        Qt::Widgets
        PkgConfig::FFMPEG
        PkgConfig::GST
        ${X11_LIBRARIES}
)