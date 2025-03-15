cmake_minimum_required(VERSION 3.16)

project(bijectif LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(EXIF REQUIRED libexif)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui LinguistTools Multimedia Sql Widgets)

if (Qt6_VERSION VERSION_LESS 6.4)
    message(FATAL_ERROR "Qt 6.4 or higher is required. You are using version ${Qt6_VERSION}.")
endif()

qt_standard_project_setup()

add_compile_definitions(
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_FROM_BYTEARRAY
    QT_NO_CAST_TO_ASCII
    QT_NO_CONTEXTLESS_CONNECT
    QT_NO_FOREACH
    QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    QT_NO_URL_CAST_FROM_STRING
    QT_STRICT_ITERATORS
    QT_USE_QSTRINGBUILDER
)

file(GLOB SRC_FILES src/*)

if (WIN32)
    set(app_icon_resource_windows "resources/bijectif.rc")
    qt_add_executable(bijectif ${SRC_FILES} ${app_icon_resource_windows})
else()
    qt_add_executable(bijectif ${SRC_FILES})
endif()

qt_add_resources(bijectif "resources"
    PREFIX "/"
    FILES resources/map.png
)

target_include_directories(bijectif SYSTEM PUBLIC ${EXIF_INCLUDE_DIRS})
target_compile_options(bijectif PUBLIC ${EXIF_CFLAGS_OTHER})

set_target_properties(bijectif PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(bijectif PRIVATE
    ${EXIF_LIBRARIES}
    Qt6::Concurrent
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Sql
    Qt6::Widgets
)

qt_add_translations(bijectif TS_FILES
    translations/bijectif_fr.ts
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

install(TARGETS bijectif)
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/bijectif/translations)
