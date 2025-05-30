# Sierra Breeze Enhanced

**Ported to Plasma 6 (community mantained)**

Be aware that this README file needs an update.

## Overview

Sierra Breeze Enhanced started as a fork of Breeze Enhanced decoration. It has the following main features:

 * Button style options: Plasma / Gnome / macOS Sierra / macOS Dark Aurorae / SBE Sierra themes / SBE Dark Aurorae themes / Color Symbols themes / Monochrome Symbols themes (Note: the application menu button is considered special and does not change).
 * Button spacing and padding Options.
 * Button hovering animation.
 * Option to make all button symbols to appear at unison on hovering (Note: it does not apply to symbol themes).
 * Titlebar style options: SBE own style of Line Separation between Titlebar and Window / Match Titlebar color to Window color / Hide Titlebar under certain circumstances (Never/Maximization/Any Maximization (including H/V)/Always) / Gradient Adjustments / Opacity Adjustments.
 * Specific Shadow settings for inactive windows
 
 
### Screenshot of SBE Sierra theme (or How it All started...)


![Active Buttons](screenshots/ActiveButtons.gif?raw=true "Active Buttons")
![Inactive Buttons](screenshots/InactiveButtons.gif?raw=true "Inactive Buttons")


### Screenshot of Settings


![SBE Settings](screenshots/SBE_settings.png?raw=true "SBE Settings")


## Installation

Please note that after installing, you need to restart KWin by executing either `kwin_x11 --replace` or `kwin_wayland --replace` in krunner (depending on whether your session runs upon X11 or Wayland). Alternatively, restarting the KDE session is obviously also an option. Then, Sierra Breeze Enhanced will appear in *System Settings &rarr; Application Style &rarr; Window Decorations*.

### Method 1: Install prebuilt packages
- Ubuntu:
```sh
sudo add-apt-repository ppa:krisives/sierrabreezeenhanced
sudo apt update
sudo apt install sierrabreezeenhanced
```
- openSUSE:
```sh
sudo zypper ar obs://home:trmdi trmdi
sudo zypper in SierraBreezeEnhanced
```
- Arch Linux:
```
git clone https://aur.archlinux.org/kwin-decoration-sierra-breeze-enhanced-git.git
cd kwin-decoration-sierra-breeze-enhanced-git
makepkg -si
cd ..
rm -rf kwin-decoration-sierra-breeze-enhanced-git
```

- Alpine Linux:
``` shell
sudo echo "http://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories
sudo apk update
sudo apk add sierrabreezeenhanced
```

### Method 2: Compile from source code
*Compilation should NOT be done against versions of Plasma < 6.3\*

#### Step 1: Build dependencies

- Ubuntu
``` shell
sudo apt install build-essential libkf6config-dev libkdecorations2-dev qtdeclarative6-dev extra-cmake-modules libkf6guiaddons-dev libkf6configwidgets-dev libkf6windowsystem-dev libkf6coreaddons-dev libkf6iconthemes-dev gettext cmake
```
- Arch Linux
``` shell
sudo pacman -S base-devel # Required development packages
sudo pacman -S kdecoration qt6-declarative    # Decoration
sudo pacman -S cmake extra-cmake-modules                    # Installation
```
- Fedora
``` shell
sudo dnf install cmake extra-cmake-modules kf6-kiconthemes-devel
sudo dnf install "cmake(Qt6Core)" "cmake(Qt6Gui)" "cmake(Qt6DBus)" "cmake(KF6GuiAddons)" "cmake(KF6WindowSystem)" "cmake(KF6I18n)" "cmake(KDecoration3)" "cmake(KF6CoreAddons)" "cmake(KF6ConfigWidgets)"
sudo dnf install qt6-qt5compat-devel kf6-kcmutils-devel qt6-qtbase-private-devel
```

- Alpine Linux
``` shell
sudo apk add extra-cmake-modules qt6-qtbase-dev qt6-qt5compat-dev kcmutils-dev kdecoration-dev kcoreaddons-dev kguiaddons-dev kconfigwidgets-dev kwindowsystem-dev ki18n-dev kiconthemes-dev
```

#### Step 2: Then compile and install
- Install from script:
```sh
chmod +x install.sh
./install.sh
```
- Or more manually:
Open a terminal inside the source directory and do:
```sh
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DKDE_INSTALL_LIBDIR=lib -DBUILD_TESTING=OFF -DKDE_INSTALL_USE_QT_SYS_PATHS=ON
make
sudo make install
```


## Uninstall

- Method 1: Use your Package manager
- Method 2: Run the uninstall script
```sh
chmod +x uninstall.sh
./uninstall.sh
```
- Method 3: or manually if previously ran the install script
```sh
cd build
sudo make uninstall
```


## Credits
Breeze, Sierra Breeze and Breeze Enhanced for obvious reasons :)
