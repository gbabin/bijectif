qt_add_library(location STATIC
    location.h location.cpp
)

target_include_directories(location INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(location PRIVATE
    Qt::Core
)
