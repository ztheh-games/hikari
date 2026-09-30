[![Build Status](https://travis-ci.org/zackthehuman/hikari.svg?branch=master)](https://travis-ci.org/zackthehuman/hikari)

![Hikari](http://zackthehuman.com/images/hikari/hikari-logo.png)

Project Hikari
==============

Project Hikari is the code name for a yet-to-be-named open-source Mega Man/Rockman game written in C++. The goal of this project is to create a faithful clone of the classic NES title ["Mega Man"][2] ("Rockman" in Japan). The feature set of the game consists of elements from various Mega Man titles, but is strongly influenced by Mega Man 3.

Gameplay and feature videos can be seen on [hakaselabshikari's YouTube page][7].

You can also contact us via our [Facebook fan page][8], if that's your kind of thing.

## Customization & Extension ##

The game is designed to be as data-driven as possible and as such it allows for customization and extension.

* Animations (sprites) are defined in `JSON` files
* Items, Enemies, Projectiles, etc. are defined in `JSON` files
* Item and Enemy behaviors are scriptable using [Squirrel][6]
* Maps/stages are defined in `JSON` files
* Music and sound effects are played from `NSF` files, and multiple NSF files can be used to mix and match music from different files

Most things are customizable. Not _everything_, but most things.

## Building ##

Project Hikari uses CMake 4.4.3 to generate platform and compiler-specific build files.

### Dependencies ###

By default, CMake downloads pinned, compatible versions of these dependencies:

* [SFML 3.1.0][4]
* [PhysicsFS 3.2.0][5]

Configure with `-DHIKARI_FETCH_DEPENDENCIES=OFF` to use installed copies instead.

SFML 3 requires a C++17-compliant compiler.

### Building on Windows (VS2010+) ###

1. Install CMake 4.4.3 and Visual Studio with C++ support.
2. Clone the repository.

        git clone https://github.com/zackthehuman/hikari.git hikari

3. Generate the build files.

        cmake -S hikari -B hikari-build

4. Build the application.

        cmake --build hikari-build --config Release

### Building on Linux (Makefile) ###

1. Install CMake 4.4.3 and a C++17 compiler.
2. Clone the repository.

        git clone https://github.com/zackthehuman/hikari.git hikari

3. Generate the build files.

        cmake -S hikari -B hikari-build -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release

4. Build the application.

        cmake --build hikari-build

### Building on Mac (Makefile) ###

1. Install CMake 4.4.3 and a C++17 compiler. When using system dependencies, SFML must be built with the same version of `libc++` as Hikari.
2. Clone the repository.

        git clone https://github.com/zackthehuman/hikari.git hikari

3. Generate the build files.

        cmake -S hikari -B hikari-build -G "Unix Makefiles" -D CMAKE_BUILD_TYPE=Release

4. Build the application.

        cmake --build hikari-build

## Why is it called "Hikari"? ##

"Hikari" is the Japanese word for "light". Since [Dr. Light][1] is the creator of Mega Man, we figured that a word associated with him would be an appropriate code name for a project like this. Originally "hikari" was going to be the name of the engine used to power the game, but focus has shifted away from a separate engine/game architecture.

## Are you actually making a clone of Mega Man 3? ##

No. While the mechanics of the game are very similar to Mega Man 3, this project is intended to be an original work and not an exact clone of any Mega Man title.

[1]: http://megaman.wikia.com/wiki/Dr._Light
[2]: http://en.wikipedia.org/wiki/Mega_Man
[3]: http://www.cmake.org/
[4]: http://www.sfml-dev.org/
[5]: http://icculus.org/physfs/downloads/
[6]: http://squirrel-lang.org
[7]: https://www.youtube.com/user/hakaselabshikari/videos
[8]: https://www.facebook.com/pages/Project-Hikari/341175436063199