################# dependencies #################
### FFTW
find_package(FFTW REQUIRED)

### Qt/KDE
find_package(Qt5 REQUIRED CONFIG COMPONENTS Widgets)

################# configuration #################
configure_file(config-breezecommon.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-breezecommon.h )

################# breezestyle target #################
set(breezeenhancedcommon_LIB_SRCS
    breezeboxshadowhelper.cpp
)

add_library(breezeenhancedcommon5 ${breezeenhancedcommon_LIB_SRCS})

generate_export_header(breezeenhancedcommon5
    BASE_NAME breezecommon
    EXPORT_FILE_NAME breezecommon_export.h)

target_link_libraries(breezeenhancedcommon5
    PUBLIC
        Qt5::Core
        Qt5::Gui
    PRIVATE
        ${FFTW_LIBRARIES})

set_target_properties(breezeenhancedcommon5 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})

install(TARGETS breezeenhancedcommon5 ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
