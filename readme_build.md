
== Prerequisites

GNU ARM toolchain can be downloaded from
  https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads.
We are currently using Version 7-2018-q2-update Linux 64-bit.

Make is usually available in some base development package, but you
can install it directly, e.g. for debian like systems
  sudo apt-get install make

Basic shell utilities (usually present in system):
  bash, dd, sed, tac, printf, find, etc.

Some usually available aux utilities (could require separate installation):
  crc32, sha1sum



== Build

Add ARM toolchain bin/ directory to PATH.
(e.g. ~/arm/gcc-arm-none-eabi-7-2018-q2-update/bin)

Run make to build the program.

Generated program 
  build/SDKdemo.pgm

Contents of QSPI
  build/SDKdemo_qspi.bin



== SDKdemo repository

The latest version of SDKdemo is available at
  https://github.com/swissmicros/SDKdemo



== SDKdemo help file

You can copy the help/sdkdemo.html file to directory /HELP/ on calculator filesystem.
Then the help is available directly on calculator by pressing F1 key.

