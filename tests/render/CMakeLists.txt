# qt_add_executable(gst-rend-test
#     gst-rend-test.cpp
# )

# qt_add_qml_module(gst-rend-test
#     URI ${GT_QML_MOD_URI_PREFIX}Test.Render
#     VERSION ${GT_QML_MOD_VERSION_MAJOR}.${GT_QML_MOD_VERSION_MINOR}
#     QML_FILES
#         qml/RendTest.qml
# )

# set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/tests)

# target_link_libraries(gst-rend-test
#     PRIVATE Qt6::Quick PkgConfig::GST
# )

gt_register_test(
  NAME rend-filesrc
  SRC rend-filesrc.cpp
  USE_QT
  QML_URI Render.Filesrc
  QML_FILES qml/RendTest.qml
  DEPS
    vid-render
    Qt6::Quick
    PkgConfig::GST
)

gt_register_test(
  NAME rend-appsrc
  SRC rend-appsrc.cpp
  USE_QT
  QML_URI Render.Appsrc
  QML_FILES qml/RendTest.qml
  DEPS
    vid-render
    Qt6::Quick
    PkgConfig::GST
)
