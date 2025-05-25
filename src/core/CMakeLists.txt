find_package(PkgConfig REQUIRED)
pkg_check_modules(EXIF REQUIRED libexif)

qt_add_library(core STATIC
    command_delete.cpp
    command_edit.cpp
    command_id.cpp
    command_insert.cpp
    command_paste.cpp
    completermodel.cpp
    filegroup.cpp
    loader.cpp
    location.cpp
    model.cpp
    modelitem.cpp
)

target_include_directories(core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(core SYSTEM PRIVATE ${EXIF_INCLUDE_DIRS})
target_compile_options(core PRIVATE ${EXIF_CFLAGS_OTHER})

target_link_libraries(core PRIVATE
    ${EXIF_LIBRARIES}
    Qt::Gui
    Qt::Multimedia
    Qt::Sql
)
