find_package(libheif REQUIRED)

qt_add_library(heifexif STATIC
    heifexif.h heifexif.cpp
)

target_include_directories(heifexif INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(heifexif PRIVATE
    heif
    Qt::Core
)
