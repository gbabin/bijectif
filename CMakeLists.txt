cmake_minimum_required(VERSION 3.16)

project(bijectif LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui LinguistTools Multimedia Sql Widgets)
qt_standard_project_setup()

qt_add_executable(bijectif
    src/main.cpp
    src/command_delete.h src/command_delete.cpp
    src/command_edit.h src/command_edit.cpp
    src/command_id.h src/command_id.cpp
    src/command_insert.h src/command_insert.cpp
    src/command_paste.h src/command_paste.cpp
    src/delegate.h src/delegate.cpp
    src/model.h src/model.cpp
    src/modelitem.h src/modelitem.cpp
    src/window.h src/window.cpp
)

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
