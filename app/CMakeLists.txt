# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
# set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

# add_subdirectory(conf)
# add_subdirectory(utils)
# add_subdirectory(vid_render)

qt_add_executable(${GT_APP_NAME}
    main.cpp
)

qt_add_qml_module(${GT_APP_NAME}
    URI Gentau.UI
    VERSION ${GT_QML_MOD_VERSION_MAJOR}.${GT_QML_MOD_VERSION_MINOR}
    QML_FILES
        qml/Main.qml
)

message("Building ${GT_APP_NAME} version ${PROJECT_VERSION}")

set_target_properties(${GT_APP_NAME} PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${GT_APP_NAME}
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS ${GT_APP_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)