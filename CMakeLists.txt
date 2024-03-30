project(sierrabreezeenhanced)
set(PROJECT_VERSION "2.0.1")
set(PROJECT_VERSION_MAJOR 0)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

include(WriteBasicConfigVersionFile)
include(FeatureSummary)

set(QT_MAJOR_VERSION 6)
set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.0.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH}
                      ${ECM_KDE_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ECMInstallIcons)
include(KDEInstallDirs6)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(GenerateExportHeader)
include(KDEClangFormat)
# include(GtkUpdateIconCache)

find_package(KDecoration2 REQUIRED)

if(NOT WIN32 AND NOT APPLE)
  find_package(KF6KCMUtils ${KF6_MIN_VERSION})
  set_package_properties(
    KF6KCMUtils PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Helps create configuration modules"
    PURPOSE
      "KCMUtils used for the configuration modules or the decoration and Qt Style"
  )
endif()

# old stuff
add_definitions(-DTRANSLATION_DOMAIN="breeze_kwin_deco")

find_package(
  KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS IconThemes CoreAddons GuiAddons
                                             ConfigWidgets WindowSystem I18n)
find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED
             COMPONENTS Widgets DBus)
find_package(Qt6 REQUIRED COMPONENTS Core5Compat)

# XCB
find_package(XCB COMPONENTS XCB)
set_package_properties(
  XCB PROPERTIES
  DESCRIPTION "X protocol C-language Binding"
  URL "https://xcb.freedesktop.org/"
  TYPE OPTIONAL
  PURPOSE "Required to pass style properties to native Windows on X11 Platform")

if(UNIX AND NOT APPLE)

  set(BREEZE_HAVE_X11 ${XCB_XCB_FOUND})

else()

  set(BREEZE_HAVE_X11 FALSE)

endif()

# ################ configuration #################
configure_file(config-breeze.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config-breeze.h)

# ################ includes #################
add_subdirectory(libbreezecommon)

# ################ newt target #################
# plugin classes
set(sierrabreezeenhanced_SRCS
    breezebutton.cpp breezedecoration.cpp breezeexceptionlist.cpp
    breezesettingsprovider.cpp breezesizegrip.cpp)

kconfig_add_kcfg_files(sierrabreezeenhanced_SRCS breezesettings.kcfgc)

# config classes they are kept separately because they might move in a separate
# library in the future
set(sierrabreezeenhanced_config_SRCS
    config/breezeconfigwidget.cpp config/breezedetectwidget.cpp
    config/breezeexceptiondialog.cpp config/breezeexceptionlistwidget.cpp
    config/breezeexceptionmodel.cpp config/breezeitemmodel.cpp)

set(sierrabreezeenhanced_config_PART_FORMS
    config/ui/breezeconfigurationui.ui
    config/ui/breezeexceptiondialog.ui config/ui/breezeexceptionlistwidget.ui)

ki18n_wrap_ui(sierrabreezeenhanced_config_PART_FORMS_HEADERS
              ${sierrabreezeenhanced_config_PART_FORMS})

# build library
add_library(
  sierrabreezeenhanced MODULE
  ${sierrabreezeenhanced_SRCS} ${sierrabreezeenhanced_config_SRCS}
  ${sierrabreezeenhanced_config_PART_FORMS_HEADERS})

target_link_libraries(
  sierrabreezeenhanced
  PUBLIC Qt6::Core Qt6::Gui Qt6::DBus Qt6::Core5Compat
  PRIVATE sierrabreezeenhancedcommon6
          KDecoration2::KDecoration
          KF6::IconThemes
          KF6::ConfigCore
          KF6::CoreAddons
          KF6::ConfigWidgets
          KF6::GuiAddons
          KF6::I18n
          KF6::KCMUtils
          KF6::WindowSystem)

if(BREEZE_HAVE_X11)
  target_link_libraries(sierrabreezeenhanced PUBLIC Qt6::GuiPrivate XCB::XCB)
endif()

install(TARGETS sierrabreezeenhanced
        DESTINATION ${KDE_INSTALL_PLUGINDIR}/${KDECORATION_PLUGIN_DIR})
# install(FILES config/sierrabreezeenhancedconfig.desktop DESTINATION
# ${SERVICES_INSTALL_DIR})
kcmutils_generate_desktop_file(sierrabreezeenhanced)

# install(TARGETS breezedecoration DESTINATION
# ${PLUGIN_INSTALL_DIR}/org.kde.kdecoration2) install(FILES
# config/breezedecorationconfig.desktop DESTINATION  ${SERVICES_INSTALL_DIR})

add_subdirectory(config)
