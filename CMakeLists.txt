cmake_minimum_required(VERSION 4.4.3)

project(hikari LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# setup version numbers
set(HIKARI_VERSION_MAJOR 0)
set(HIKARI_VERSION_MINOR 2)
set(HIKARI_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(HIKARI_GOOD_COMPILER_FLAGS "-Wall -Wpedantic -O -g")
set(HIKARI_DUMB_COMPILER_FLAGS "-W3")

option(HIKARI_FETCH_DEPENDENCIES
    "Download pinned dependencies instead of using system packages"
    ON
)

if(HIKARI_FETCH_DEPENDENCIES)
    include(FetchContent)

    if(WIN32)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    endif()

    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SDL3
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.16.tar.gz
        URL_HASH SHA256=c2ee715e42ec520c4d11fd8d249ef1d2b2baf4ad31148b72b3b000276c0b3633
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SDL3)

    set(SDLIMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
    set(SDLIMAGE_PNG_LIBPNG OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SDL3_image
        URL https://github.com/libsdl-org/SDL_image/archive/refs/tags/release-3.4.6.tar.gz
        URL_HASH SHA256=0bbac8c799273488ce32c4a53a44fc0fc70a6079458bd9d46fc4e72da138367f
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SDL3_image)

    set(PHYSFS_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(PHYSFS_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(PHYSFS_BUILD_TEST OFF CACHE BOOL "" FORCE)
    set(PHYSFS_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(PHYSFS_DISABLE_INSTALL ON CACHE BOOL "" FORCE)

    # PhysicsFS 3.2.0 predates CMake 4's removal of policy compatibility
    # below 3.5, so provide the supported policy floor for this dependency.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
    FetchContent_Declare(
        PhysFS
        URL https://github.com/icculus/physfs/archive/refs/tags/release-3.2.0.tar.gz
        URL_HASH SHA256=1991500eaeb8d5325e3a8361847ff3bf8e03ec89252b7915e1f25b3f8ab5d560
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(PhysFS)
    unset(CMAKE_POLICY_VERSION_MINIMUM)
endif()

if(NOT TARGET SDL3::SDL3)
    find_package(SDL3 3.4.16 EXACT CONFIG REQUIRED)
endif()
if(NOT TARGET SDL3_image::SDL3_image)
    find_package(SDL3_image 3.4.6 EXACT CONFIG REQUIRED)
endif()

include(cmake/HikariShaders.cmake)
enable_testing()

#
# MSVC-specific settings
#
# Remove security warnings with Visual C++
#
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HIKARI_DUMB_COMPILER_FLAGS}")

    MESSAGE(STATUS "==== MSVC detected - Adding MSVC-specific compiler flags")

    if(NOT CMAKE_CXX_FLAGS MATCHES "/EHsc")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    endif()

    add_definitions( -D_CRT_SECURE_NO_DEPRECATE )
    add_definitions( -D_CRT_SECURE_NO_WARNINGS )
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HIKARI_GOOD_COMPILER_FLAGS}")

    #
    # Clang-specific settings
    #
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      MESSAGE(STATUS "==== Clang detected - Adding Clang-specific compiler flags")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
      set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++" )
      set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++" )
      set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++" )
    endif()
endif()

#
# Apple-specific stuff
#
if( APPLE )
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++" )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++" )
  set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++" )
endif( APPLE )

add_subdirectory( engine )
add_subdirectory( tests )
