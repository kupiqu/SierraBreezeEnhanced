set(breezedecoration_config_SRCS
    ../breezeexceptionlist.cpp
    breezeconfigwidget.cpp
    breezedetectwidget.cpp
    breezeexceptiondialog.cpp
    breezeexceptionlistwidget.cpp
    breezeexceptionmodel.cpp
    breezeitemmodel.cpp)
ki18n_wrap_ui(
  breezedecoration_config_SRCS ui/breezeconfigurationui.ui
  ui/breezeexceptiondialog.ui ui/breezeexceptionlistwidget.ui)

kcoreaddons_add_plugin(
  kcm_sierrabreezeenhanceddecoration SOURCES kcm_sierrabreezeenhanceddecoration
  ${breezedecoration_config_SRCS} INSTALL_NAMESPACE
  "${KDECORATION_KCM_PLUGIN_DIR}")
kconfig_add_kcfg_files(kcm_sierrabreezeenhanceddecoration
                       ../breezesettings.kcfgc)
target_include_directories(kcm_sierrabreezeenhanceddecoration
                           PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/)
target_link_libraries(
  kcm_sierrabreezeenhanceddecoration
  PUBLIC Qt6::Core Qt6::Gui Qt6::DBus Qt6::Core5Compat Qt6::GuiPrivate
  PRIVATE KF6::ConfigCore KF6::CoreAddons KF6::GuiAddons KF6::I18n
          KF6::KCMUtils KF6::WindowSystem)
if(BREEZE_HAVE_X11)
  target_link_libraries(kcm_sierrabreezeenhanceddecoration
                        PUBLIC Qt6::GuiPrivate XCB::XCB)
endif()
kcmutils_generate_desktop_file(kcm_sierrabreezeenhanceddecoration)
