# Age of Empires Free Software Remake

*All copyrighted material and trademarks belong to their respective owners. This
is an educational non-profit project we are writing in our free time.*

## Overview

Linux native game port of windows game Age of Empires Rise of Rome

It tries to take fully advantage of POSIX while mimicking the windows API and
the genie game engine for compatibility reasons. In order to achieve this, some
code is reconstructed from the dissassembly. We have provided a couple of tools
that verify, test, and inspect the original data files.

You still need to own an original legitimate copy of Age of Empires in order to
run this program, since you need to specify the location of the original data
directory.

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

This project has been built primarly for Xubuntu 14.04.5, but it should build on
any POSIX-compatible machine.

For more information, see [INSTALL](INSTALL).

## Troubleshooting

Make sure you own a legitimate copy of the original Age of Empires or Rise of
Rome expansion and you have specified the root directory to AoE (RoR) correctly.
See [known issues and bugs](BUGS.md) for more details.

## Links

* [The project](https://www.github.com/folkertvanverseveld/aoe) on Github.
* [Issue tracker](https://www.github.com/folkertvanverseveld/aoe/issues) on Github.
* [Related projects](related.md)
* [Order the game from Amazon](https://duckduckgo.com/?q=age+of+empires+!amazon&t=canonical&ia=web)
