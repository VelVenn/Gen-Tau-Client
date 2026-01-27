qt_add_executable(gst-rend-test
    gst-rend-test.cpp
)

qt_add_qml_module(gst-rend-test
    URI Gentau.Test.Render
    VERSION ${QML_MODULE_VERSION_MAJOR}.${QML_MODULE_VERSION_MINOR}
    QML_FILES
        qml/RendTest.qml
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/tests)

target_link_libraries(gst-rend-test
    PRIVATE Qt6::Quick PkgConfig::GST
)

# include(GNUInstallDirs)
# install(TARGETS gst-rend-test
#     BUNDLE DESTINATION .
#     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
# )