cmake_minimum_required(VERSION 4.4.3)

project(hikari LANGUAGES C CXX)

# setup version numbers
set(HIKARI_VERSION_MAJOR 0)
set(HIKARI_VERSION_MINOR 2)
set(HIKARI_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
set(HIKARI_GOOD_COMPILER_FLAGS "-std=c++11 -Wall -Wpedantic -O -g")
set(HIKARI_DUMB_COMPILER_FLAGS "-W3")

if(WIN32)
    # always link statically on Windows
    set(SFML_STATIC_LIBRARIES TRUE)
endif()

option(HIKARI_FETCH_DEPENDENCIES
    "Download pinned dependencies instead of using system packages"
    ON
)

if(HIKARI_FETCH_DEPENDENCIES)
    include(FetchContent)

    if(WIN32)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    endif()

    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SFML
        URL https://github.com/SFML/SFML/archive/refs/tags/2.6.2.tar.gz
        URL_HASH SHA256=15ff4d608a018f287c6a885db0a2da86ea389e516d2323629e4d4407a7ce047f
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SFML)

    if(WIN32)
        if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
            set(HIKARI_OPENAL_ARCH ARM64)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(HIKARI_OPENAL_ARCH x64)
        else()
            set(HIKARI_OPENAL_ARCH x86)
        endif()
        set(HIKARI_OPENAL_RUNTIME
            "${sfml_SOURCE_DIR}/extlibs/bin/${HIKARI_OPENAL_ARCH}/openal32.dll"
        )
    endif()

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
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
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
