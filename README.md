# Brány Skeldalu (Gates of Skeldal)

The platform independed port of the game Gates Of Skeldal.

WORK IN PROGRESS!!!

# Disclaimer on Licensing

**The source code of the game is licensed under the MIT License.**

**However, the game assets, including but not limited to graphical files, audio files, maps, texts, video content, and similar materials, are protected by copyright law. Free distribution is permitted solely for the source code. Distribution of the game assets is not allowed without explicit permission.**

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

The `skeldal.ini` must be at current folder or you can specify config using command line option

```
$ ./skeldal
$ ./skeldal -f conf/skeldal.ini
```

The documentation of the configuration file is in the file.

### Fix scripting bug in BILA_VEZ.MAP (White Tower)

A script error in the White Tower map in the puzzle located on the top floor of the tower has been fixed. The repository includes a corrected file BILA_VEZ.MAP, replace the original file in the "MAPS" directory of the original installation with this file.

This fix will only work with the new code. (It will not work in earlier releases of the game, i.e. in the DOS, Windows, Android and iOS versions). The reason is that a new scripting action has been introduced to enable this fix, which will ensure the correct evaluation of the puzzle

## Controller support

Controller support is experimental. Tested on PS4 controller connected on bluetooth. 
Controller can be configured in INI file. If it is connected when program starts,
it is detected and can be used. 

Following mapping is defined by default (you can change it in INI)
Left stick - walking turning, strafing with meta, list selection
Right stick -  cursor movement
Key X - cursor action (left mouse click)
Key [] - ENTER/RETURN, accept selection, start battle, finish move
Key O - cancel action, walk by cursor, (right mouse click)
Key /\ - SPACE, wall action, fast trade, next PC in battle, accept selection
Key Ps - settings
Key Share - SAVE
Key Options - LOAD
Key on right stick - Split group
Key R1 - mod/meta key
Key L1 - with meta - sleep, without - backspace
Key DPAD UP - map, with meta cast spell
Key DPAD DOWN - merge group
Key DPAD left - move left
KEY DPAD right - move right
Key X,[],O,/\ with meta - actions during battle


## Goals

 1. to rewrite all Intel 386 depend code to independed variant.
 2. tp rewrite all ASM code to C
 3. tp improve C code by using up C20 features (original is C89)
 4. a new code should be written in C++20
 5. to fix all bugs, to run under valgrind and to use other tools to find bugs
 6. to render using SDL - Fullscreen and Windowed z
 7. to implement sounds and music using SDL sound library
 8. to define and polish platform API - to allow future ports.
 9. Install/Setup GUI application - by using some platform independed library
10. Target platforms: Windows, Ubuntu/Debian/Linux, MacOS.
11. ADV (custom adventure) support for existing adventures
12.  - later MapEdit and other tools

## Considered changes in the game
1) Campaigns - the player will have saved games sorted by campaign and the number of saved positions in a campaign will not be limited. The only limitation will be the number of campaigns to 10, as the graphics only allow for 10 positions.
2) Autosave should be always on. There will be 1 autosave position
3) Console/Cheat interface - there should be console enabled for everyone available under a shortcut key - for example Shift+Ctrl+C
4) Death screen redesign


## Changes and improvements in compare to orginal game

 1. It is possible to split a group and leave part of the group in another map. For now, it is not possible to switch between groups in different maps, you have to return to the map for the disconnected members and reconnect them. Alternatively, you can use the "Merge 3" spell
 2. Leaving a dead member on the map does not make them completely lost. It is possible to return to the map later and revive them.
 3. The "Reincarnation" spell can be used to revive characters lost due to falling into the abyss, drowning, burning, or any other means where the character's corpse is out of reach. In this case, the revived character is automatically transported to the caster's location.
 4. The "Merge 3" spell also applies to all character corpses
 5. The "Dexterity" attribute is on the opposite side of "Mobility" because it is primarily intended for use in shooting, where shooting takes longer (having to draw an arrow, aim, and fire) than a melee attack. To support this attribute, the rules for two-weapon combat have been changed. In order for a character to hold and fight with two weapons, the sum of the required attributes of both weapons must be met. For example, weapons with strength requirements of 20 and 30 mean having a strength of 50 to wield both. However, if a character has "Dexterity", they can use this attribute instead of the required attribute of the other weapon, with the higher number of the pair being considered, while still having to meet the requirements for both weapons individually. So to hold both weapons in the example above, they would need "Strength" = 30 and "Dexterity = 30"
 6. Attacking with two melee weapons causes two attacks in one action.
 7. When making a melee attack, the higher number from "Strength" and "Dexterity" is used as the attack attribute. However, this does not mean that a character with higher Dexterity can wield a weapon with a higher "Strength" attribute requirement. (Finesse rule)


## Console commands

Console is available by pressing Ctrl+Shift+C during exploration and battle programming mode

### switches
```
inner-eye       on/off
hunter-instinct on/off
no-hassle       on/off
iron-skin       on/off
ghost-walls     on/off
true-seeing     on/off
walking-in-air  on/off
enemy-insight   on/off
ghost-form      on/off
```

### actions

#### gold

```
i-require-gold
to-the-moon
offlers-blessing
```

#### characters

```
rise-and-shine
ascent
by-the-power-of-grayskull
dispel-magic
```

#### maps

```
global-map
echo-location
echoes-of-the-past
```

### with parameters

```
locate <string>
summon <string>
say <string>
speed <integer>
portal-jump <string.map>
talk <integer>
talk </integer>
unachieve <achievement>
unachieve all

```

### debug

```
flute
status
steam
ls
cat <file>
hex <file>
```
