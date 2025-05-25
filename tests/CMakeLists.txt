qt_add_executable(test_model
    settings.cpp
    test_model.cpp
)

set_target_properties(test_model PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(test_model PRIVATE
    core
    Qt::Gui
    Qt::Sql
    Qt::Test
)

qt_add_executable(test_row
    settings.cpp
    test_row.cpp
)

set_target_properties(test_row PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(test_row PRIVATE
    core
    Qt::Gui
    Qt::Sql
    Qt::Test
)

enable_testing()

add_test(NAME test_model COMMAND test_model)
add_test(NAME test_row COMMAND test_row)
