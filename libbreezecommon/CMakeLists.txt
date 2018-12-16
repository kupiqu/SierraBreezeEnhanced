################# dependencies #################
### Qt/KDE
find_package(Qt5 REQUIRED CONFIG COMPONENTS Widgets)

################# configuration #################
configure_file(config-breezecommon.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-breezecommon.h )

################# breezestyle target #################
set(sierrabreezeenhancedcommon_LIB_SRCS
    breezeboxshadowrenderer.cpp
)

add_library(sierrabreezeenhancedcommon5 ${sierrabreezeenhancedcommon_LIB_SRCS})

generate_export_header(sierrabreezeenhancedcommon5
    BASE_NAME breezecommon
    EXPORT_FILE_NAME breezecommon_export.h)

target_link_libraries(sierrabreezeenhancedcommon5
    PUBLIC
        Qt5::Core
        Qt5::Gui)

set_target_properties(sierrabreezeenhancedcommon5 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS sierrabreezeenhancedcommon5 ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
