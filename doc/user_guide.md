# Age of Empires User Guide

*This guide is far from complete. Text is subject to change at any time.*

## Building the game

This project does not provide binary builds at the moment, which means you have
to build the project in order to play the game.

Install instructions on Linux can be found in [INSTALL](../INSTALL).

On Windows, you can either install the dependencies manually or use Microsoft's
[vcpkg](https://github.com/microsoft/vcpkg/tags). This is the one we will be
using here.

After downloading vcpkg, open an administrator CMD prompt in the vcpkg directory
and type:
```
.\bootstrap-vcpkg.bat -disableMetrics
vcpkg integrate install
```

Close the prompt and open a unprivileged one:
```
vcpkg install gtest sdl2 sdl2-image sdl2-mixer[mpg123]
```

## Running the game

All game data required to run the game is located on the CD-ROM. The CD-ROM
should be auto-detected if you start the game after inserting the CD-ROM.

If the CD-ROM is not recognized, make sure you have inserted the Age of Empires
CD-ROM correctly and that the CD-ROM is mounted properly.

Make sure the original game installer has been run to ensure the game fonts
are installed or copy the game fonts manually. Eventually, we hope to support
the game trial edition and automate this process.

### Creating an ISO image

It is possible to create an ISO image from a mixed-mode CD-ROM using cdrdao. You
need to create a so called TOC/BIN or CUE/BIN pair. It is recommended to use a
TOC/BIN pair. You can create a TOC/BIN pair like this:
```
sudo cdrdao read-cd --read-raw --datafile IMAGE.bin --driver generic-mmc:0x20000 --device /dev/CDROM IMAGE.toc
```
Replace IMAGE with the name of the image to create (e.g. aoe) and CDROM with the
cdrom device. On Xubuntu, the CD-ROM device is sr0.

If you cannot mount the IMAGE.toc file, you can convert it to IMAGE.cue like
this:
```
toc2cue IMAGE.toc IMAGE.cue
```

### Mounting an ISO image

Most linux distributions provide out-of-the-box mounting in the file manager or
using a third-party tool. On Xubuntu, we recommend using cdemu which can be
installed by adding a PPA.

Mounting with cdemu can usually be done in your file manager, but you can also
do it manually using:
```
cdemu 0 IMAGE.cue
```

If it does not mount properly, you can try this after running cdemu:
```
sudo mount -t iso9660 /dev/cdemu/0 /mnt
```

Replace /mnt with the directory you want to mount to.

### Installing cdemu on Xubuntu

Before installing cdemu, we would like to warn you that **adding PPAs can break
your package manager!**

On Xubuntu, add the PPA like this and press ENTER to confirm:
```
sudo add-apt-repository ppa:cdemu/ppa
```
Then, update your packages and install cdemu:
```
sudo apt-get update
sudo apt-get install gcdemu
```
