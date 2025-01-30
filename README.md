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

You need to create following file in the root of the game folder 

**WSKELDAL.INI**

```
CESTA_MAPY ./maps/
CESTA_MUSIC ./music/
CESTA_VIDEO ./video/
CESTA_DATA ./
CESTA_POZICE ./savegame/
CESTA_TEMPY /tmp/
vmode 0
sound_device 9 0 0 0
SOUND_MIXFREQ 44100
default_map lespred.map
ZOOM_SPEED 8
TURN_SPEED 8
MUSIC_VOLUME 127
SOUND_VOLUME 255
PRELOAD 0
AUTOSAVE 8
WINDOWED 1
SKIP_INTRO 8
FULLRESVIDEO 0
DEBUG 0
GAME_SPEED 6
BATTLE_ACCEL 0
WINDOWEDZOOM 0
EXTRAS 0
```
(the final format of the configuration file is subject of change)

Move to root folder of the original game and run the built binary 

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



