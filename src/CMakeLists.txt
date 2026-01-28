set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(conf)
add_subdirectory(utils)
add_subdirectory(vid_render)

qt_add_executable(${APP_NAME}
    main.cpp
)

qt_add_qml_module(${APP_NAME}
    URI Gentau.UI
    VERSION ${QML_MODULE_VERSION_MAJOR}.${QML_MODULE_VERSION_MINOR}
    QML_FILES
        app/qml/Main.qml
)

message("Building ${APP_NAME} version ${PROJECT_VERSION}")

set_target_properties(${APP_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${APP_NAME}
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS ${APP_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)