# Project status

*The current state of the project is documented here. At the moment, it cannot
do anything meaningful yet.*

All items are in random order unless stated otherwise.

## Working

These items are implemented in either the genie game engine or the game:

Subsystem   | Task / Purpose          | Notes
------------|-------------------------|----------------------------------------------------
Video       | AVI video playback      | Uses command-line interface for VLC
Bugreport   | Debug info for crashes  | Crash handler in case the engine or game crashes
CD-ROM      | File I/O                | Known issues regarding auto-mounting audio tracks
Shell       | Command line interface  | Monospaced white on black only font
File I/O    | Parse basic game assets | Audio works, parsing other types is being worked on
Tools       | (Un)packing game assets | Pretty stable, but the code needs a good cleanup
Game assets | PE parsing              | Integrated libpe in empires and aoesetup

Unfortunately, the audio tracks mounting issue is related to the particular
Linux distribution the game is running on and we do not know if this is possible
to fix at all.

## In progress

These items are being worked on:

Subsystem      | Task / Purpose        | Notes
---------------|-----------------------|-------------------------------------
Empires folder | Replicate and port    | Basic graphics and audio
"       "      | old game and launcher | subsystem working
User Interface | True Type Fonts       | Porting to new launcher
"    "         | Menu navigation       | Basic buttons and group navigation
"    "         | Main menu             | See next task
"    "         | Background image      | Requires lots of reverse engineering
Game assets    | Asset parsing         | Huge task and very time consuming

## Backlog

These items will be picked up when other items are being completed:

Subsystem      | Task / Purpose   | Notes
---------------|------------------|-----------------------------------------------------------
Multiplayer    | Networking       | For multiplayer games etc.
Other stuff    | Configuration    | Either on compile time (./configure) or runtime (~/.aoerc)

## On hold

These items are more troublesome, boring or difficult than other items. Some may
require more investigation or the scope of the issue has to be clarified.

Subsystem | Task / Purpose | Notes
----------|----------------|----------------------------------------------
Engine    | MIDI playback  | Old launcher is being migrated to the new one
