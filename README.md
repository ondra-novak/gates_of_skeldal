# Brány Skeldalu (Gates of Skeldal)

The platform independed port of the game Gates Of Skeldal. 

WORK IN PROGRESS!!!

## BUILD

Required SDL 2.0
Base development platform is Ubuntu 24

```
mkdir build
cd build
cmake ..
make all
```

## RUN

You need original DOS version files (distrubuted on CD) or Windows port files.

You need to create or update `skeldal.ini` following file in the root of the game folder 

The `skeldal.ini` must be at current folder or you can specify config using command line option

```
$ ./skeldal
$ ./skeldal -f conf/skeldal.ini
```

The documentation of the configuration file is in the file.

## todo

1)  to rewrite all Intel 386 depend code to independed variant.
2)  tp rewrite all ASM code to C
3)  tp improve C code by using up C20 features (original is C89)
4)  a new code should be written in C++20
5)  to fix all bugs, to run under valgrind and to use other tools to find bugs
6)  to render using SDL - Fullscreen and Windowed
7)  to implement sounds and music using SDL sound library
8)  to define and polish platform API - to allow future ports.
9)  Install/Setup GUI application - by using some platform independed library
10)  Target platforms: Windows, Ubuntu/Debian/Linux, MacOS.
11)  ADV (custom adventure) support for existing adventures
12)  - later MapEdit and other tools

## considered changes in the game
1) Campaigns - the player will have saved games sorted by campaign and the number of saved positions in a campaign will not be limited. The only limitation will be the number of campaigns to 10, as the graphics only allow for 10 positions.
2) Autosave should be always on. There will be 1 autosave position
3) Console/Cheat interface - there should be console enabled for everyone available under a shortcut key - for example Shift+Ctrl+C
4) Death screen redesign



