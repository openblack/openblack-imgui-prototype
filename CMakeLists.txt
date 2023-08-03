cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

# VCPKG has an imperfect android implementation.
# NDK must be in env and vcpkg_android.cmake selects architecture.
if (VCPKG_TARGET_ANDROID)
  set(ENV{ANDROID_NDK_HOME} ${ANDROID_NDK_HOME})
  include(vcpkg_android)
endif ()

project(openblack-imgui-prototype)

# Configure C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# don't allow in source builds
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# do find_package with CONFIG before MODULE
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

# Special case for SDL2 dependency, goal is to find a config that exports SDL2::SDL2 target
# libsdl2-dev has a `sdl2-config.cmake` that doesn't export this, but vcpkg does..
find_package(SDL2 CONFIG QUIET)
if (NOT TARGET SDL2::SDL2)
  find_package(SDL2 MODULE REQUIRED)
endif ()

find_package(Stb QUIET)
if (NOT Stb_FOUND)
  find_path(
    Stb_INCLUDE_DIR
    NAMES stb_image_write.h stb_image.h stb_rect_pack.h
    PATHS /usr/include/stb REQUIRED
  )
endif ()

find_package(imgui REQUIRED)

add_executable(openblack-imgui-prototype main.cpp)
target_link_libraries(openblack-imgui-prototype SDL2::SDL2 imgui::imgui)
target_include_directories(openblack-imgui-prototype PRIVATE ${Stb_INCLUDE_DIR})
