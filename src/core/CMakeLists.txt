find_package(PkgConfig REQUIRED)
pkg_check_modules(EXIF REQUIRED libexif)

qt_add_library(core STATIC
    command_delete.h command_delete.cpp
    command_edit.h command_edit.cpp
    command_id.h command_id.cpp
    command_insert.h command_insert.cpp
    command_paste.h command_paste.cpp
    completermodel.h completermodel.cpp
    filegroup.h filegroup.cpp
    filekind.h
    loader.h loader.cpp
    memorycache.h memorycache.cpp
    model.h model.cpp
    modelitem.h modelitem.cpp
    pyramid.h pyramid.cpp
    settings.h
)

target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(core SYSTEM PRIVATE ${EXIF_INCLUDE_DIRS})
target_compile_options(core PRIVATE ${EXIF_CFLAGS_OTHER})

target_link_libraries(core PRIVATE
    heifexif
    location
    video
    QGeoView
    ${EXIF_LIBRARIES}
    Qt::Gui
    Qt::Network
    Qt::Sql
    Qt::Widgets
)
