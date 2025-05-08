cmake_minimum_required(VERSION 3.16)

project(tester LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(EXIF REQUIRED libexif)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui Multimedia Sql Test Widgets)

qt_add_executable(tester
    ../src/command_delete.h ../src/command_delete.cpp
    ../src/command_edit.h ../src/command_edit.cpp
    ../src/command_id.h ../src/command_id.cpp
    ../src/command_insert.h ../src/command_insert.cpp
    ../src/command_paste.h ../src/command_paste.cpp
    ../src/completermodel.h ../src/completermodel.cpp
    ../src/delegate.h ../src/delegate.cpp
    ../src/filegroup.h ../src/filegroup.cpp
    ../src/loader.h ../src/loader.cpp
    ../src/location.h ../src/location.cpp
    ../src/model.h ../src/model.cpp
    ../src/modelitem.h ../src/modelitem.cpp
    ../src/settings.h
    main.cpp
    settings.cpp
    test_model.h test_model.cpp
    test_row.h test_row.cpp
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
