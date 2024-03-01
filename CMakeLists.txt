cmake_minimum_required(VERSION 3.16)

project(bijectif LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui LinguistTools Multimedia Sql Widgets)

qt_standard_project_setup()

add_compile_definitions(
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_FROM_BYTEARRAY
    QT_NO_CAST_TO_ASCII
    QT_NO_URL_CAST_FROM_STRING
    QT_USE_QSTRINGBUILDER
)

file(GLOB SRC_FILES src/*)

if (WIN32)
    set(app_icon_resource_windows "resources/bijectif.rc")
    qt_add_executable(bijectif ${SRC_FILES} ${app_icon_resource_windows})
else()
    qt_add_executable(bijectif ${SRC_FILES})
endif()

set_target_properties(bijectif PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(bijectif PRIVATE
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
