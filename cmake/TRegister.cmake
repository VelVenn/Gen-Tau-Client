# ============================================================================
# Module registration function
# ============================================================================
function(gt_register_mod)
  set(options POSITION_INDEPENDENT GEN_ANCHOR)
  set(oneValueArgs NAME TYPE ALIAS)
  set(multiValueArgs SRC DEPS)

  cmake_parse_arguments(PARSE_ARGV 0 GT_MOD "${options}" "${oneValueArgs}" "${multiValueArgs}")

  if(NOT GT_MOD_NAME)
    message(FATAL_ERROR "!! gt_register_mod -> Module name not specified !!")
  else()
    message(STATUS "gt_register_mod -> ${GT_MOD_NAME}: Registering module")
  endif()

  # 检查模块是否已注册
  get_property(already_registered GLOBAL PROPERTY GEN_TAU_MODULES_${GT_MOD_NAME} DEFINED)
  if(already_registered)
    message(FATAL_ERROR "!! gt_register_mod -> ${GT_MOD_NAME}: Module already registered !!")
  endif()

  if(NOT GT_MOD_TYPE)
    message(FATAL_ERROR "!! gt_register_mod -> ${GT_MOD_NAME}: Module type not specified !!")
  endif()

  if(NOT GT_MOD_TYPE MATCHES "^(STATIC|INTERFACE|SHARED)$")
    message(
      FATAL_ERROR
      "!! gt_register_mod -> ${GT_MOD_NAME}: Invalid module type ${GT_MOD_TYPE}, must be STATIC, INTERFACE or SHARED !!"
    )
  else()
    message(STATUS "gt_register_mod -> ${GT_MOD_NAME}: Module type set to '${GT_MOD_TYPE}'")
  endif()

  # 设置模块名称
  set(REAL_MOD_NAME ${GT_LIB_PREFIX}${GT_MOD_NAME})

  # 生成模块别名
  if(NOT GT_MOD_ALIAS)
    string(REPLACE "-" ";" NAME_PARTS ${GT_MOD_NAME})
    set(PASCAL_NAME "")
    foreach(PART ${NAME_PARTS})
      string(SUBSTRING ${PART} 0 1 FIRST_CHAR)
      string(TOUPPER ${FIRST_CHAR} FIRST_CHAR)
      string(SUBSTRING ${PART} 1 -1 REST_CHARS)
      string(APPEND PASCAL_NAME "${FIRST_CHAR}${REST_CHARS}")
    endforeach()
    set(GT_MOD_ALIAS ${GT_EXPORT_LIB_NS}${PASCAL_NAME})

    message(STATUS "gt_register_mod -> ${GT_MOD_NAME}: No alias specified, setting as '${GT_MOD_ALIAS}'")
  endif()

  # 设置模块全局属性
  set_property(GLOBAL APPEND PROPERTY GEN_TAU_MODULES ${GT_MOD_NAME})
  set_property(GLOBAL PROPERTY GEN_TAU_MODULES_${GT_MOD_NAME} ${REAL_MOD_NAME})

  # 设置模块类别
  if(GT_MOD_TYPE STREQUAL "INTERFACE")
    add_library(${REAL_MOD_NAME} INTERFACE)
  else()
    if(NOT GT_MOD_SRC)
      message(FATAL_ERROR "!! gt_register_mod -> ${GT_MOD_NAME}: SOURCE required for ${GT_MOD_TYPE} module !!")
    endif()

    add_library(${REAL_MOD_NAME} ${GT_MOD_TYPE} ${GT_MOD_SRC})
  endif()

  # 设置模块别名
  add_library(${GT_MOD_ALIAS} ALIAS ${REAL_MOD_NAME})

  # 设置模块头文件目录
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/inc")
    if(GT_MOD_TYPE STREQUAL "INTERFACE")
      target_include_directories(${REAL_MOD_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
      )
    else()
      target_include_directories(${REAL_MOD_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
      )
    endif()
    message(STATUS "gt_register_mod -> ${GT_MOD_NAME}: Include directory set to 'inc/'")
  else()
    message(WARNING "! gt_register_mod -> ${GT_MOD_NAME}: No 'inc/' directory found !")
  endif()

  # 解析模块依赖
  set(LIB_DEPS_CUR_SCOPE "PUBLIC")
  set(LIB_PUBLIC_DEPS "")
  set(LIB_PRIVATE_DEPS "")
  set(LIB_INTERFACE_DEPS "")

  foreach(dep ${GT_MOD_DEPS})
    if(dep STREQUAL "PUBLIC" OR dep STREQUAL "PRIVATE" OR dep STREQUAL "INTERFACE")
      set(LIB_DEPS_CUR_SCOPE "${dep}")
    else()
      get_property(internal_mods GLOBAL PROPERTY GEN_TAU_MODULES_${dep})

      if(internal_mods)
        list(APPEND LIB_${LIB_DEPS_CUR_SCOPE}_DEPS "${internal_mods}")
        message(STATUS "gt_register_mod -> ${GT_MOD_NAME}: Resolved '${dep}' -> '${internal_mods}'")
      else()
        list(APPEND LIB_${LIB_DEPS_CUR_SCOPE}_DEPS "${dep}")
      endif()
    endif() 
  endforeach()

  # 链接依赖
  if(GT_MOD_TYPE STREQUAL "INTERFACE" AND (LIB_PUBLIC_DEPS OR LIB_PRIVATE_DEPS))
    message(FATAL_ERROR "!! gt_register_mod -> ${GT_MOD_NAME}: INTERFACE module cannot have PUBLIC or PRIVATE dependencies !!")
  endif()

  if(LIB_PUBLIC_DEPS)
    target_link_libraries(${REAL_MOD_NAME} PUBLIC ${LIB_PUBLIC_DEPS})
  endif()

  if(LIB_PRIVATE_DEPS)
    target_link_libraries(${REAL_MOD_NAME} PRIVATE ${LIB_PRIVATE_DEPS})
  endif()

  if(LIB_INTERFACE_DEPS)
    target_link_libraries(${REAL_MOD_NAME} INTERFACE ${LIB_INTERFACE_DEPS})
  endif()

  # 位置无关代码
  if(GT_MOD_POSITION_INDEPENDENT AND NOT GT_MOD_TYPE STREQUAL "INTERFACE")
    set_target_properties(${REAL_MOD_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()

  # 生成clangd锚点
  if(GT_MOD_GEN_ANCHOR)
    # 检查是否存在 clangd-anchor.cpp
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/clangd-anchor.cpp")
      add_library(
        gt_${GT_MOD_NAME}_clangd_anchor STATIC EXCLUDE_FROM_ALL
        clangd-anchor.cpp
      )

      target_link_libraries(
        gt_${GT_MOD_NAME}_clangd_anchor
        PRIVATE ${REAL_MOD_NAME}
      )

      message(STATUS "gt_register_mod -> ${GT_MOD_NAME}: Clangd anchor target created")
    else()
      message(WARNING
        "! gt_register_mod -> ${GT_MOD_NAME}: GEN_ANCHOR specified but 'clangd-anchor.cpp' not found !\n"
        "!                    Please create it or remove GEN_ANCHOR option. !"
      )
    endif()
  endif()

  # 注册完成
  message(STATUS "✓ Module registered: ${GT_MOD_NAME} -> ${REAL_MOD_NAME} (${GT_MOD_ALIAS}) [${GT_MOD_TYPE}]")

endfunction()

# ============================================================================
# Test registration function
# ============================================================================
function(gt_register_test)
  set(options USE_QT USE_CTEST)
  set(oneValueArgs NAME QML_URI)
  set(multiValueArgs SRC QML_FILES DEPS)

  cmake_parse_arguments(PARSE_ARGV 0 GT_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}")

  if(NOT GT_TEST_NAME)
    message(FATAL_ERROR "!! gt_register_test -> Test name not specified !!")
  else()
    message(STATUS "gt_register_test -> ${GT_TEST_NAME}: Registering test")
  endif()

  # 检查测试是否已注册
  get_property(already_registered GLOBAL PROPERTY GEN_TAU_TESTS_${GT_TEST_NAME} DEFINED)
  if(already_registered)
    message(FATAL_ERROR "!! gt_register_test -> ${GT_TEST_NAME}: Test already registered !!")
  endif()

  # 设置测试全局属性
  set_property(GLOBAL APPEND PROPERTY GEN_TAU_TESTS ${GT_TEST_NAME})
  set_property(GLOBAL PROPERTY GEN_TAU_TESTS_${GT_TEST_NAME} ${GT_TEST_NAME})

  if(NOT GT_TEST_SRC)
    message(FATAL_ERROR "!! gt_register_test -> ${GT_TEST_NAME}: Source not specified !!")
  endif()

  # 创建可执行文件
  if(GT_TEST_USE_QT)
    qt_add_executable(${GT_TEST_NAME} ${GT_TEST_SRC})
    
    if(GT_TEST_QML_FILES)
      if(NOT GT_TEST_QML_URI)
        set(GT_TEST_QML_URI ${GT_TEST_NAME})
        message(STATUS "gt_register_test -> ${GT_TEST_NAME}: QML URI not specified, setting same as the test name")
      endif()

      qt_add_qml_module(${GT_TEST_NAME}
        URI ${GT_QML_MOD_URI_PREFIX}Test.${GT_TEST_QML_URI}
        VERSION ${GT_QML_MOD_VERSION_MAJOR}.${GT_QML_MOD_VERSION_MINOR}
        QML_FILES ${GT_TEST_QML_FILES}
      )

      message(STATUS "gt_register_test -> ${GT_TEST_NAME}: QML URI specified as '${GT_QML_MOD_URI_PREFIX}Test.${GT_TEST_QML_URI}'")
    endif()
  else()
    if(GT_TEST_QML_FILES OR GT_TEST_QML_URI)
      message(WARNING "! gt_register_test -> ${GT_TEST_NAME}: QML files or URI specified but USE_QT not enabled !")
    endif()

    add_executable(${GT_TEST_NAME} ${GT_TEST_SRC})
  endif()

  # 设置输出目录
  set_target_properties(
    ${GT_TEST_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${GT_TEST_OUTPUT_PATH}
  )
  
  # 解析依赖
  foreach(dep ${GT_TEST_DEPS})
    get_property(internal_mod GLOBAL PROPERTY GEN_TAU_MODULES_${dep})
    if(internal_mod)
      list(APPEND RESOLVED_DEPS ${internal_mod})
      message(STATUS "gt_register_test -> ${GT_TEST_NAME}: Resolved '${dep}' -> '${internal_mod}'")
    else()
      list(APPEND RESOLVED_DEPS ${dep})
    endif()
  endforeach()
  
  if(RESOLVED_DEPS)
    target_link_libraries(${GT_TEST_NAME} PRIVATE ${RESOLVED_DEPS})
  endif()
  
  # 添加到 CTest
  if(GT_TEST_USE_CTEST)
    add_test(NAME ${GT_TEST_NAME} COMMAND ${GT_TEST_NAME})
  endif()

  # 注册完成
  message(STATUS "✓ Test registered: ${GT_TEST_NAME} [Qt:${GT_TEST_USE_QT}, CTest:${GT_TEST_USE_CTEST}]")

endfunction()