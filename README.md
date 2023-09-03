# bmos
This repo follows the tutorial ["Building an OS"](https://www.youtube.com/watch?v=9t-SPC7Tczc&list=PLFjM7v6KGMpiH2G-kT781ByCNC_0pKpPN) on the ["Nanobyte"](https://www.youtube.com/channel/UCSPIuWADJIMIf9Erf--XAsA) YouTube channel.


ASSUMPTIONS:
    x86 CPU
    familiarity with bytes, words, doubles, and quads, and binary, hexadecimal, Little Endian, and Assembly Language

REQUIREMENTS:

* A Linux environment for development.
    Native Ubuntu, Arch, etc., or
    Windows Subsystem for Linux (WSL), or
    Cygwin

* An assembler, to generate machine code.
    `sudo apt install nasm`

* Make, to facilitate the build process.
    `sudo apt install make`

* A virtualization environment, to execute the code.
    `sudo apt install qemu`

* A graphical debugger, to see what's going wrong!
    `sudo apt install bochs bochs-sdl bochsbios vgabios`

nice to have:

*   Git + GitHub
*   An IDE, such as VSCode (preferred)
*   VSCode plugins for Git, Assembly Language, and viewing Binary Files

***
*** BUILDING
***

    `make all`
    or just plain
    `make`    

***
*** RUNNING
***
    Your BIOS (or emulator) must enable Legacy Booting instead of UEFI

    `make run`
    or
    `make debug`



