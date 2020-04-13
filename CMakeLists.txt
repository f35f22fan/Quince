cmake_minimum_required(VERSION 3.10)
#set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Don't use clang because:
# https://stackoverflow.com/questions/56317596/glm-fails-to-build-when-using-clion
set(CMAKE_C_COMPILER "/usr/bin/clang-9")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-9")

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


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4 gstreamer-app-1.0>=1.4)
include_directories(${GST_INCLUDE_DIRS})

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

foreach(path ${CMAKE_PREFIX_PATH})
    message("Path: " ${path})
endforeach(path)

message(STATUS "C++ Compiler: " ${CMAKE_CXX_COMPILER})
message(STATUS "Qt5Core: " ${Qt5Core_VERSION} ", Gui: "
 ${Qt5Gui_VERSION} ", Widgets: " ${Qt5Widgets_VERSION})

set(src_files
    actions.hxx
    App.cpp App.hpp
    decl.hxx
    Duration.cpp Duration.hpp
    GstPlayer.cpp GstPlayer.hpp
    gui/decl.hxx
    gui/SongItem.cpp gui/SongItem.hpp
    gui/Table.cpp gui/Table.hpp
    gui/TableModel.cpp gui/TableModel.hpp
    io/io.cc io/io.hh io/io.hxx
    main.cpp err.hpp
    types.hxx)

foreach(f IN LISTS src_files)
	get_filename_component(b ${f} NAME)
	set_source_files_properties(${f} PROPERTIES
		COMPILE_DEFINITIONS "SRC_FILE_NAME=\"${b}\"")
endforeach()


set(exe_name "app")

add_executable(${exe_name} ${src_files} resources.qrc)
target_link_libraries(${exe_name} Qt5::Core Qt5::Gui Qt5::Widgets ${dl_lib}
    ${GST_LIBRARIES} rt ${GST_CFLAGS})
# rt for clock_monotonic_raw

