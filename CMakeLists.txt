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
find_package(unofficial-nativefiledialog CONFIG REQUIRED)

add_executable(openblack-imgui-prototype main.cpp)
add_compile_definitions(openblack-imgui-prototype SDL_MAIN_HANDLED)
target_link_libraries(openblack-imgui-prototype SDL2::SDL2 imgui::imgui)
if (NOT EMSCRIPTEN)
  target_link_libraries(openblack-imgui-prototype unofficial::nativefiledialog::nfd)
endif ()
target_include_directories(openblack-imgui-prototype PRIVATE ${Stb_INCLUDE_DIR})

if (EMSCRIPTEN)
  # Must be run from project root using `emrun --no_browser --serve_after_close --serve_after_exit .`
  # Browser should point to http://localhost:6931/emscripten-vcpkg/bin/Debug/openblack.html?-g&relative/path/to/game/files
  set_target_properties(
          openblack-imgui-prototype
          PROPERTIES
          COMPILE_FLAGS_DEBUG "-g3 -gsource-map"
          LINK_FLAGS_DEBUG "-g3 -gsource-map"
          COMPILE_FLAGS "-s USE_SDL=2 -fexceptions"
          LINK_FLAGS
          "-s EXPORTED_FUNCTIONS=\"['_main', '_malloc']\" -s EXPORTED_RUNTIME_METHODS=ccall -s ALLOW_MEMORY_GROWTH=1 -s USE_SDL=2 -s FULL_ES3=1 -s GL_ASSERTIONS=1 -s USE_WEBGL2=1 -fexceptions --emrun"
          SUFFIX ".html"
  )
endif ()
