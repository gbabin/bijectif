qt_add_library(KDSignalThrottler STATIC
    KDSignalThrottler.h KDSignalThrottler.cpp
)

target_include_directories(KDSignalThrottler INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(KDSignalThrottler PRIVATE
    Qt::Core
)
