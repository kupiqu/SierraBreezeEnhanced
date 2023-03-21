
project(sierrabreezeenhanced)
set(PROJECT_VERSION "1.3.3")
set(PROJECT_VERSION_MAJOR 0)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

include(WriteBasicConfigVersionFile)
include(FeatureSummary)

find_package(ECM 0.0.9 REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake)

include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(GenerateExportHeader)
# include(GtkUpdateIconCache)

find_package(KDecoration2 REQUIRED)


# old stuff
add_definitions(-DTRANSLATION_DOMAIN="breeze_kwin_deco")

find_package(KF5 REQUIRED COMPONENTS CoreAddons GuiAddons ConfigWidgets WindowSystem I18n IconThemes)
find_package(Qt5 CONFIG REQUIRED COMPONENTS DBus)

### XCB
find_package(XCB COMPONENTS XCB)
set_package_properties(XCB PROPERTIES
  DESCRIPTION "X protocol C-language Binding"
  URL "https://xcb.freedesktop.org/"
  TYPE OPTIONAL
  PURPOSE "Required to pass style properties to native Windows on X11 Platform"
)

if(UNIX AND NOT APPLE)

  set(BREEZE_HAVE_X11 ${XCB_XCB_FOUND})
  if (XCB_XCB_FOUND)
    find_package(Qt5 REQUIRED CONFIG COMPONENTS X11Extras)
  endif()

else()

  set(BREEZE_HAVE_X11 FALSE)

endif()

################# configuration #################
configure_file(config-breeze.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-breeze.h )

################# includes #################
add_subdirectory(libbreezecommon)

################# newt target #################
### plugin classes
set(sierrabreezeenhanced_SRCS
    breezebutton.cpp
    breezedecoration.cpp
    breezeexceptionlist.cpp
    breezesettingsprovider.cpp
    breezesizegrip.cpp)

kconfig_add_kcfg_files(sierrabreezeenhanced_SRCS breezesettings.kcfgc)

### config classes
### they are kept separately because they might move in a separate library in the future
set(sierrabreezeenhanced_config_SRCS
    config/breezeconfigwidget.cpp
    config/breezedetectwidget.cpp
    config/breezeexceptiondialog.cpp
    config/breezeexceptionlistwidget.cpp
    config/breezeexceptionmodel.cpp
    config/breezeitemmodel.cpp
)

set(sierrabreezeenhanced_config_PART_FORMS
   config/ui/breezeconfigurationui.ui
   config/ui/breezedetectwidget.ui
   config/ui/breezeexceptiondialog.ui
   config/ui/breezeexceptionlistwidget.ui
)

ki18n_wrap_ui(sierrabreezeenhanced_config_PART_FORMS_HEADERS ${sierrabreezeenhanced_config_PART_FORMS})

### build library
add_library(sierrabreezeenhanced MODULE
    ${sierrabreezeenhanced_SRCS}
    ${sierrabreezeenhanced_config_SRCS}
    ${sierrabreezeenhanced_config_PART_FORMS_HEADERS})

target_link_libraries(sierrabreezeenhanced
    PUBLIC
        Qt5::Core
        Qt5::Gui
        Qt5::DBus
    PRIVATE
        sierrabreezeenhancedcommon5
        KDecoration2::KDecoration
        KF5::ConfigCore
        KF5::CoreAddons
        KF5::ConfigWidgets
        KF5::GuiAddons
        KF5::I18n
        KF5::IconThemes
        KF5::WindowSystem)

if(BREEZE_HAVE_X11)
  target_link_libraries(sierrabreezeenhanced
    PUBLIC
      Qt5::X11Extras
      XCB::XCB)
endif()


install(TARGETS sierrabreezeenhanced DESTINATION ${PLUGIN_INSTALL_DIR}/org.kde.kdecoration2)
install(FILES config/sierrabreezeenhancedconfig.desktop DESTINATION  ${SERVICES_INSTALL_DIR})
# install(TARGETS breezedecoration DESTINATION ${PLUGIN_INSTALL_DIR}/org.kde.kdecoration2)
# install(FILES config/breezedecorationconfig.desktop DESTINATION  ${SERVICES_INSTALL_DIR})
