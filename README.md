# SierraBreezeEnhanced

## Overview

SierraBreezeEnhanced is a fork of BreezeEnhanced decoration with the following changes:

 * non-gray colors do not change.
 * active window: show symbol on hovering.
 * inactive window: always show symbol, show ring color on hovering.
 * application menu button is considered special and stays as in vanilla breeze.
 * no more option for non macOS-like buttons as it doesn't apply anymore.
 * added, however, an option to either choose active vs. inactive style (default), always active style (also for non-active windows), or inactive style (also for the active window).
 * horizontal padding to better adjust the button position in case of use of rounded corners.

## Credits:

SierraBreezeEnhanced is strongly based on BreezeEnhanced and SierraBreeze.

## Installation

*Compilation should not be done against versions of KWin < 5.14.*

Open a terminal inside the source directory and do:
```sh
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_LIBDIR=lib -DBUILD_TESTING=OFF -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
make
sudo make install
```
After the intallation, restart KWin by logging out and in. Then, SierraBreezeEnhanced will appear in *System Settings &rarr; Application Style &rarr; Window Decorations*.

### Ubuntu PPA

Users of Ubuntu based distros (such as KDE Neon) can add the PPA and install the package by:

```sh
sudo add-apt-repository ppa:krisives/sierrabreezeenhanced
sudo apt update
sudo apt install sierrabreezeenhanced
```

## Screenshots:

![Active Buttons](screenshots/ActiveButtons.gif?raw=true "Active Buttons")
![Inactive Buttons](screenshots/InactiveButtons.gif?raw=true "Inactive Buttons")
![Inactive Buttons](screenshots/symbol-style.gif?raw=true "Symbol Style")
