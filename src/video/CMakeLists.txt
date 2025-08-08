find_package(PkgConfig REQUIRED)
pkg_check_modules(FFmpeg REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswscale
)

qt_add_library(video STATIC
    video.h video.cpp
)

target_include_directories(video INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(video PRIVATE
    Qt::Gui
    PkgConfig::FFmpeg
)
