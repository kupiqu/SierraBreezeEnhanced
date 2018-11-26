# SierraBreezeEnhanced

## Overview

SierraBreezeEnhanced is a fork of BreezeEnhanced decoration with the following changes:

 * non-gray colors do not change.
 * active window: show symbol on hovering.
 * inactive window: always show symbol, show ring color on hovering.
 * application menu button is considered special and stays as in vanilla breeze.
 * no more option for non macOS-like buttons as it doesn't apply anymore.
 * added, however, an option to either choose active vs. inactive style (default), always active style (also for non-active windows), or inactive style (also for the active window).

## Credits:

SierraBreezeEnhanced is strongly based on BreezeEnhanced and SierraBreeze.

## Installation

The version number in the file NEWS shows the main version of KWin that is required for the compilation. *Compilation should not be done against other versions of KWin!*.

Open a terminal inside the source directory and do:
```sh
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_LIBDIR=lib -DBUILD_TESTING=OFF -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
make
sudo make install
```
After the intallation, restart KWin by logging out and in. Then, SierraBreezeEnhanced will appear in *System Settings &rarr; Application Style &rarr; Window Decorations*.

## Screenshots:

![Active Buttons](screenshots/ActiveButtons.gif?raw=true "Active Buttons")
![Inactive Buttons](screenshots/InactiveButtons.gif?raw=true "Inactive Buttons")
