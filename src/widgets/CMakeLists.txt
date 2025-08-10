qt_add_library(widgets STATIC
    custommapxyz.h custommapxyz.cpp
    delegate.h delegate.cpp
    tablevieweventfilter.h tablevieweventfilter.cpp
    viewer.h viewer.cpp
    window.h window.cpp
)

target_include_directories(widgets INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(widgets PRIVATE
    core
    location
    KDSignalThrottler
    QGeoView
    Qt::Concurrent
    Qt::Network
    Qt::Sql
    Qt::Widgets
)
