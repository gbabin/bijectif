qt_add_library(widgets STATIC
    delegate.cpp
    window.cpp
)

target_include_directories(widgets INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(widgets PRIVATE
    core
    KDSignalThrottler
    QGeoView
    Qt::Concurrent
    Qt::Network
    Qt::Sql
    Qt::Widgets
)
