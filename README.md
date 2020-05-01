# Age of Empires Free Software Remake

*All copyrighted material and trademarks belong to their respective owners. This
is an educational non-profit project we are writing in our free time.*

## Preview

![Main Menu](https://user-images.githubusercontent.com/5989565/66706421-8fe7ed80-ed32-11e9-9a4c-3cdac94ab0c4.png)
![Demo map](https://user-images.githubusercontent.com/5989565/66708315-eb27d900-ed4e-11e9-8b0f-737848cfccc0.png)

## Overview

Cross-platform free software game port of Microsoft's original Age of Empires
and the Rise of Rome expansion.

It tries to take fully advantage of modern technology and APIs while mimicking
the windows API and the genie game engine for compatibility reasons. In order to
achieve this, some code is reconstructed from the dissassembly. We have provided
a couple of tools that verify, test, and inspect the original data files.

You still need to own an original legitimate copy of Age of Empires in order to
run this program, since you need the original game data to run the game.

Community free assets are not supported at the moment, but there are tools
included if you want to make your own. It is not recommended to create your own
at the moment because we haven't figured out what all resources are needed for
and what dimensions, colors etc. the game expects.

## User Guide

A simple, incomplete user guide is available [here](doc/user_guide.md).

## Status

At the moment, there is no game to be played. Only basic functionality and lots
of simple low-level stuff in the game engine is implemented. See
[STATUS](STATUS.md) for more details.

## Support

*NOTE* empires/empires is considered legacy and is going to be replaced by
empires/empiresx eventually! empires/empires is more feature complete at the
moment, but empiresx is an improved version that provides multiplayer support
as well.

Operating System    | Supported programs and tools
--------------------|-----------------------------
Xubuntu 18.04.4     | AoEx, server, tools
Windows 10 Pro 1909 | AoEx, server

The programs and tools are abbreviated as:

Abbreviation | Program(s)/Tool(s) | Path(s)
-------------|--------------------|--------------------------
AoEx         | Age of Empires     | empiresx/empiresx
server       | Standalone server  | empiresx/dedicated_server
tools        | Mod tools          | tools/

For more information how to install and run AoE, see [INSTALL](INSTALL).

## Troubleshooting

Make sure you own a legitimate copy of the original Age of Empires or Rise of
Rome expansion and you have specified the root directory to AoE (RoR) correctly.
See [known issues and bugs](BUGS.md) for more details.

## Links

* [The project](https://www.github.com/folkertvanverseveld/aoe) on Github.
* [Issue tracker](https://www.github.com/folkertvanverseveld/aoe/issues) on Github.
* [Related projects](related.md)
* [Order the game from Amazon](https://duckduckgo.com/?q=age+of+empires+!amazon&t=canonical&ia=web)
