project(QuincePlayer)
cmake_minimum_required(VERSION 3.10)
#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Don't use clang because:
# https://stackoverflow.com/questions/56317596/glm-fails-to-build-when-using-clion
set(CMAKE_C_COMPILER "/usr/bin/clang-10")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-10")

# SET (CMAKE_C_COMPILER             "/usr/bin/clang")
# SET (CMAKE_C_FLAGS                "-Wall -std=c99")
# SET (CMAKE_C_FLAGS_DEBUG          "-g")
# SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
# SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
# SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

# SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
# SET (CMAKE_CXX_FLAGS                "-Wall")
# SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
# SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
# SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
# SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# SET (CMAKE_AR      "/usr/bin/llvm-ar")
# SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
# SET (CMAKE_NM      "/usr/bin/llvm-nm")
# SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
# SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")

#link_directories(${QuincePlayer}/libs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4
    gstreamer-plugins-base-1.0 gstreamer-pbutils-1.0)
include_directories(${GST_INCLUDE_DIRS})

pkg_check_modules(FLAC REQUIRED flac++>=1.3)
include_directories(${FLAC_INCLUDE_DIRS})

pkg_check_modules(OPUSFILE REQUIRED opusfile)
include_directories(${OPUSFILE_INCLUDE_DIRS})

find_package(KF5GlobalAccel)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

foreach(path ${CMAKE_PREFIX_PATH})
    message("Path: " ${path})
endforeach(path)

message(STATUS "C++ Compiler: " ${CMAKE_CXX_COMPILER})
message(STATUS "Qt5Core: " ${Qt5Core_VERSION} ", Gui: "
 ${Qt5Gui_VERSION} ", Widgets: " ${Qt5Widgets_VERSION})

set(src_files
    actions.hxx
    audio.cc audio.hh audio.hxx
    audio/decl.hxx
    audio/Meta.cpp audio/Meta.hpp
    audio/TempSongInfo.hpp
    App.cpp App.hpp
    ByteArray.cpp ByteArray.hpp
    decl.hxx
    Duration.cpp Duration.hpp
    GstPlayer.cpp GstPlayer.hpp
    gui/decl.hxx
    gui/Playlist.cpp gui/Playlist.hpp gui/playlist.hxx
    gui/PlaylistStackWidget.cpp gui/PlaylistStackWidget.hpp
    gui/SeekPane.cpp gui/SeekPane.hpp
    gui/Table.cpp gui/Table.hpp
    gui/TableModel.cpp gui/TableModel.hpp
    io/File.cpp io/File.hpp
    io/io.cc io/io.hh io/io.hxx
    main.cpp err.hpp
    quince.hh quince.cc
    Song.cpp Song.hpp
    types.hxx)

foreach(f IN LISTS src_files)
	get_filename_component(b ${f} NAME)
	set_source_files_properties(${f} PROPERTIES
		COMPILE_DEFINITIONS "SRC_FILE_NAME=\"${b}\"")
endforeach()

add_library(global_hotkeys_x11 SHARED shared/global_hotkeys.hpp shared/global_hotkeys_x11.cpp)
add_library(global_hotkeys_wl SHARED shared/global_hotkeys.hpp shared/global_hotkeys_wl.cpp)

set(exe_name "app")
add_executable(${exe_name} ${src_files} resources.qrc)
target_link_libraries(${exe_name} Qt5::Core Qt5::Gui Qt5::Widgets ${dl_lib}
    ${GST_LIBRARIES} rt ${GST_CFLAGS} ${FLAC_LIBRARIES} ${FLAC_CFLAGS}
    ${OPUSFILE_LIBRARIES} ${OPUSFILE_CFLAGS} KF5::GlobalAccel)
# rt for clock_monotonic_raw

