cmake_minimum_required(VERSION 3.16)

project(bijectif LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui LinguistTools Network Sql Test Widgets)

if (Qt6_VERSION VERSION_LESS 6.4)
    message(FATAL_ERROR "Qt 6.4 or higher is required. You are using version ${Qt6_VERSION}.")
endif()

qt_standard_project_setup()

add_subdirectory(src/KDSignalThrottler)

option(BUILD_EXAMPLES "Build QGeoView examples" OFF)
add_subdirectory(src/QGeoView)

add_subdirectory(src/app)
add_subdirectory(src/core)
add_subdirectory(src/heif)
add_subdirectory(src/location)
add_subdirectory(src/video)
add_subdirectory(src/widgets)
add_subdirectory(tests)

foreach(TARGET core heifexif location video widgets bijectif)
    target_compile_definitions(${TARGET} PUBLIC
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_FROM_BYTEARRAY
        QT_NO_CAST_TO_ASCII
        QT_NO_CONTEXTLESS_CONNECT
        QT_NO_FOREACH
        QT_NO_KEYWORDS
        QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
        QT_NO_URL_CAST_FROM_STRING
        QT_STRICT_ITERATORS
        QT_USE_QSTRINGBUILDER
    )
endforeach()

if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()
