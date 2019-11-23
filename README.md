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

## Build dependencies

### Ubuntu
``` shell
sudo apt install build-essential libkf5config-dev libkdecorations2-dev libqt5x11extras5-dev qtdeclarative5-dev extra-cmake-modules libkf5guiaddons-dev libkf5configwidgets-dev libkf5windowsystem-dev libkf5coreaddons-dev gettext
```

### Arch Linux
``` shell
sudo pacman -S kdecoration qt5-declarative qt5-x11extras    # Decoration
sudo pacman -S cmake extra-cmake-modules                    # Installation
```

### Fedora
``` shell
sudo dnf install cmake extra-cmake-modules
sudo dnf install "cmake(Qt5Core)" "cmake(Qt5Gui)" "cmake(Qt5DBus)" "cmake(Qt5X11Extras)" "cmake(KF5GuiAddons)" "cmake(KF5WindowSystem)" "cmake(KF5I18n)" "cmake(KDecoration2)" "cmake(KF5CoreAddons)" "cmake(KF5ConfigWidgets)"
```

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

Alternatively install from script (which does the above):
```sh
chmod +x install.sh
./install.sh
```

### Ubuntu PPA

Users of Ubuntu based distros (such as KDE Neon) can add the PPA and install the package by:

```sh
sudo add-apt-repository ppa:krisives/sierrabreezeenhanced
sudo apt update
sudo apt install sierrabreezeenhanced
```

### openSUSE package

Users of openSUSE Tumbleweed/Leap can add this repo and install the package by:

```sh
sudo zypper ar obs://home:trmdi trmdi
sudo zypper in SierraBreezeEnhanced
```

## Uninstall

Run the uninstall script
```sh
chmod +x uninstall.sh
./uninstall.sh
```
or manually if previously ran the install script
```sh
cd build
sudo make uninstall
```

## Screenshots:

![Active Buttons](screenshots/ActiveButtons.gif?raw=true "Active Buttons")
![Inactive Buttons](screenshots/InactiveButtons.gif?raw=true "Inactive Buttons")
