file(GLOB SRC_FILES CONFIGURE_DEPENDS "*.cpp")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Protobuf)

qt_add_executable(${GT_APP_NAME}
  ${SRC_FILES}
  inc/GTCommViewModel.hpp
)

qt_add_protobuf(${GT_APP_NAME}
    PROTO_FILES
        proto/topics.proto
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
  PRIVATE
  Qt6::Quick
  Qt6::Protobuf
  Gentau::Utils
  Gentau::Comm
  Gentau::ImgTrans
)

target_include_directories(${GT_APP_NAME}
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

include(GNUInstallDirs)
install(TARGETS ${GT_APP_NAME}
  BUNDLE DESTINATION .
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
