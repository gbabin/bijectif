cmake_minimum_required(VERSION 3.16)

project(bijectif LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(EXIF REQUIRED libexif)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui LinguistTools Multimedia Sql Test Widgets)

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
    QT_NO_KEYWORDS
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

target_compile_options(bijectif PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4>
)

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

enable_testing()

qt_add_executable(tester
    src/command_delete.h src/command_delete.cpp
    src/command_edit.h src/command_edit.cpp
    src/command_id.h src/command_id.cpp
    src/command_insert.h src/command_insert.cpp
    src/command_paste.h src/command_paste.cpp
    src/completermodel.h src/completermodel.cpp
    src/delegate.h src/delegate.cpp
    src/filegroup.h src/filegroup.cpp
    src/loader.h src/loader.cpp
    src/location.h src/location.cpp
    src/model.h src/model.cpp
    src/modelitem.h src/modelitem.cpp
    src/settings.h
    tests/main.cpp
    tests/settings.cpp
    tests/test_model.h tests/test_model.cpp
    tests/test_row.h tests/test_row.cpp
)

target_include_directories(tester SYSTEM PUBLIC ${EXIF_INCLUDE_DIRS})
target_compile_options(tester PUBLIC ${EXIF_CFLAGS_OTHER})

set_target_properties(tester PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(tester PRIVATE
    ${EXIF_LIBRARIES}
    Qt6::Concurrent
    Qt6::Core
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Sql
    Qt6::Test
    Qt6::Widgets
)

add_test(NAME test COMMAND tester)
