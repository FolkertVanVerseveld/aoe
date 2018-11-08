# Project status

*The current state of the project is documented here. At the moment, it can
start a dummy 4 player singleplayer game without any real game mechanics.*

All items are in random order unless stated otherwise.

## Working

Note: the genie game engine is being redesigned and integrated directly in
empires/. Once the game is playable the engine is extracted once again to
genie/. These items are implemented in either the genie game engine or the game:

Subsystem      | Task / Purpose           | Notes
---------------|--------------------------|----------------------------------------------------
CD-ROM         | File I/O                 | Known issues regarding auto-mounting audio tracks
File I/O       | Parse basic game assets  | Audio works, partial slp and bin parsing
Tools          | (Un)packing game assets  | Pretty stable, but outdated and needs some cleanup
Game assets    | PE parsing               | Integrated libpe in empires and aoesetup
User Interface | Background image parsing | Minor problems with dialogs

Unfortunately, the audio tracks mounting issue is related to the particular
Linux distribution the game is running on and we do not know if this is possible
to fix at all. Also, depending on the method you have backed up/converted the
real CD-ROM to ISO, it may play garbled audio data.

## In progress

These items are being worked on:

Subsystem       | Task / Purpose        | Notes
----------------|-----------------------|-------------------------------------
Empires folder  | Replicate and port    | Basic graphics and audio
"       "       | old game and launcher | subsystem working
User Interface  | True Type Fonts       | Porting to new launcher
"    "          | Menu navigation       | Basic buttons and group navigation
"    "          | Main menu             | See next task
DRS Game assets | Asset parsing         | Completed for palettes, backgrounds and some slps, very time consuming
Slp parsing     | Game gfx assets       | Working for backgrounds, but has glitches with most images
Game logic      | Fix bad game state    | State gets corrupted sometimes

## Backlog

These items will be picked up when other items are being completed:

Subsystem      | Task / Purpose                 | Notes
---------------|--------------------------------|-----------------------------
Shell          | Command line interface         | Migrate from old to new game
Bugreport      | Debug info for crashes         | "", crash handler in case the engine or game crashes
Video          | AVI video playback             | "", uses command-line interface for VLC
Video          | Refactor graphics renderer     | Provide plain SDL renderer and OpenGL
Tools          | Autostrip BMP and WAVE headers | Headers must be stripped to be recognized properly by the original game.
User interface | Create chatfield for commands  | Spawn units, get resources etc.
"              | Split code in more files       | Some stuff really doesn't belong here...
Global         | Refactoring some parts to C    | I'm stupid and can't get everything work fast and properly in C++...

## On hold

These items are more troublesome, boring or difficult than other items. Some may
require more investigation or the scope of the issue has to be clarified.

Subsystem      | Task / Purpose | Notes
---------------|----------------|----------------------------------------------
Multiplayer    | Networking     | For multiplayer games etc.
Engine         | MIDI playback  | Old launcher is being migrated to the new one
Other stuff    | Configuration  | Either on compile time (./configure) or runtime (~/.aoerc)
