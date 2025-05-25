set(SRC_FILES
    main.cpp
    settings.cpp
)

if (WIN32)
    set(app_icon_resource_windows "../../resources/bijectif.rc")
    qt_add_executable(bijectif ${SRC_FILES} ${app_icon_resource_windows})
else()
    qt_add_executable(bijectif ${SRC_FILES})
endif()

qt_add_resources(bijectif "resources"
    PREFIX "/"
    FILES ../../resources/map.png
)

set_target_properties(bijectif PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(bijectif PRIVATE
    core
    widgets
    Qt::Gui
    Qt::Sql
    Qt::Widgets
)

qt_add_translations(bijectif TS_FILES
    ../../translations/bijectif_fr.ts
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

install(TARGETS bijectif)
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/bijectif/translations)
