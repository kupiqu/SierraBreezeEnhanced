# ################ dependencies #################
# Qt/KDE
find_package(Qt6 REQUIRED CONFIG COMPONENTS Widgets)

# ################ configuration #################
configure_file(config-breezecommon.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config-breezecommon.h)

# ################ breezestyle target #################
set(sierrabreezeenhancedcommon_LIB_SRCS breezeboxshadowrenderer.cpp)

add_library(sierrabreezeenhancedcommon6 ${sierrabreezeenhancedcommon_LIB_SRCS})

generate_export_header(sierrabreezeenhancedcommon6 BASE_NAME breezecommon
                       EXPORT_FILE_NAME breezecommon_export.h)

target_link_libraries(sierrabreezeenhancedcommon6 PUBLIC Qt6::Core Qt6::Gui)

set_target_properties(
  sierrabreezeenhancedcommon6 PROPERTIES VERSION ${PROJECT_VERSION}
                                         SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS sierrabreezeenhancedcommon6 ${INSTALL_TARGETS_DEFAULT_ARGS}
        LIBRARY NAMELINK_SKIP)
