function(gt_extra_link_info)
  message(STATUS "")
  message(STATUS "========================================")
  message(STATUS "  Gen-Tau Module Link Information")
  message(STATUS "========================================")

  # 获取所有已注册的模块
  get_property(all_modules GLOBAL PROPERTY GEN_TAU_MODULES)

  if(NOT all_modules)
    message(STATUS "No modules registered.")
    return()
  endif()

  # 遍历每个模块
  foreach(mod ${all_modules})
    # 获取真实的 target 名称
    get_property(target GLOBAL PROPERTY GEN_TAU_MODULES_${mod})

    message(STATUS "")
    message(STATUS "Module: ${mod}")
    message(STATUS "  Target: ${target}")

    # 获取库类型
    get_target_property(type ${target} TYPE)
    message(STATUS "  Type: ${type}")

    # ===== 使用 CMake 原生属性 =====

    # INTERFACE_LINK_LIBRARIES - 包含 PUBLIC 和 INTERFACE 依赖
    get_target_property(iface_libs ${target} INTERFACE_LINK_LIBRARIES)
    if(iface_libs AND NOT iface_libs STREQUAL "iface_libs-NOTFOUND")
      message(STATUS "  INTERFACE_LINK_LIBRARIES (PUBLIC + INTERFACE):")
      foreach(lib ${iface_libs})
        message(STATUS "    - ${lib}")
      endforeach()
    else()
      message(STATUS "  INTERFACE_LINK_LIBRARIES: (none)")
    endif()

    # LINK_LIBRARIES - 包含所有依赖 (PUBLIC + PRIVATE)
    get_target_property(link_libs ${target} LINK_LIBRARIES)
    if(link_libs AND NOT link_libs STREQUAL "link_libs-NOTFOUND")
      message(STATUS "  LINK_LIBRARIES (PUBLIC + PRIVATE):")
      foreach(lib ${link_libs})
        message(STATUS "    - ${lib}")
      endforeach()
    else()
      message(STATUS "  LINK_LIBRARIES: (none)")
    endif()

    # 包含目录
    get_target_property(inc_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(inc_dirs AND NOT inc_dirs STREQUAL "inc_dirs-NOTFOUND")
      message(STATUS "  Include directories:")
      foreach(dir ${inc_dirs})
        message(STATUS "    - ${dir}")
      endforeach()
    endif()

    # PIC 设置
    get_target_property(pic ${target} POSITION_INDEPENDENT_CODE)
    if(pic)
      message(STATUS "  Position Independent Code: ${pic}")
    endif()

    # 编译选项
    get_target_property(compile_opts ${target} COMPILE_OPTIONS)
    if(compile_opts AND NOT compile_opts STREQUAL "compile_opts-NOTFOUND")
      message(STATUS "  Compile options:")
      foreach(opt ${compile_opts})
        message(STATUS "    - ${opt}")
      endforeach()
    endif()
  endforeach()

  message(STATUS "")
  message(STATUS "========================================")
  list(LENGTH all_modules module_count)
  message(STATUS "  Total modules: ${module_count}")
  message(STATUS "========================================")
  message(STATUS "")
endfunction()
