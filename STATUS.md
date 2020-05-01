# Project status

*The current state of the project is documented here. At the moment, it can
start a dummy singleplayer game without any real game mechanics.*

All items are in random order unless stated otherwise.
Binaries are not provided at this moment, because there is no game to be played.
Once the first prototype becomes stable, windows binaries and some common linux
distro binaries (e.g. Ubuntu) will be provided.

## Working

We have written a game engine that mimmicks the original genie game engine which
is integrated directly into setup/ and a more sophisticated version into
empires/. These items are implemented in either the genie game engine or the
game:

Subsystem      | Task / Purpose           | Notes
---------------|--------------------------|----------------------------------------------------
CD-ROM         | File I/O                 | Known issues regarding auto-mounting audio tracks
File I/O       | Parse basic game assets  | Audio works, partial slp and bin parsing
Tools          | (Un)packing game assets  | Pretty stable, but outdated and needs some cleanup
Game assets    | PE parsing               | Integrated libpe in empires and aoesetup. Completed for palettes, backgrounds and some slps
User Interface | Background image parsing | Minor problems with dialogs
"              | Cursor handling          | Cursor clipping, custom cursors almost work (legacy screens have issues in X11)
Slp parsing    | Game gfx assets          |
Cross-platform | Windows and Linux        |
Multiplayer    | Networking               | Basic chat lobby and dummy game, minor problems with start sync point

Unfortunately, the audio tracks mounting issue is related to the particular
Linux distribution the game is running on and we do not know if this is possible
to fix at all. Also, depending on the method you have backed up/converted the
real CD-ROM to ISO, it may play garbled audio data. We have no way to check if
the CD-ROM has no bad sectors.

## In progress

These items are being worked on:

Subsystem       | Task / Purpose        | Notes
----------------|-----------------------|-------------------------------------
Multiplayer     | Networking            | For multiplayer games etc.
User Interface  | Missing menu's etc.   |
Game logic      | Dynamic Unit logic    | Movement stub (yanked in empiresx)

## Backlog

These items will be picked up when other items are being completed:

Subsystem      | Task / Purpose                 | Notes
---------------|--------------------------------|-----------------------------
Shell          | Command line interface         | Migrate from old to new game
Bugreport      | Debug info for crashes         | "", crash handler in case the engine or game crashes
Video          | AVI video playback             | "", uses command-line interface for VLC
Tools          | Autostrip BMP and WAVE headers | Headers must be stripped to be recognized properly by the original game.
User interface | Create chatfield for commands  | Spawn units, get resources etc.
"              | Split code in more files       | Some stuff really doesn't belong here...

## On hold

These items are more troublesome, boring or difficult than other items. Some may
require more investigation or the scope of the issue has to be clarified.

Subsystem      | Task / Purpose             | Notes
---------------|----------------------------|----------------------------------------------
Engine         | MIDI playback              | Old launcher is being migrated to the new one
Other stuff    | Configuration              | Either on compile time (./configure) or runtime (~/.aoerc)
Video          | Refactor graphics renderer | Provide plain SDL renderer and OpenGL
