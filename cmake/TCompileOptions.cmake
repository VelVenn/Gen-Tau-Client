# =========================== ENABLE ADDRESS SANITIZER ===========================
option(GEN_TAU_USE_ASAN "Enable address sanitizer for all gen-τ targets (ONLY FOR GNU/LLVM compilers)" OFF)

if(GEN_TAU_USE_ASAN)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "-> Address Sanitizer enabled")
    add_compile_options(
      -fsanitize=address 
      -fno-omit-frame-pointer
      -fno-optimize-sibling-calls
      -fsanitize-address-use-after-scope
      -fsanitize-recover=address
      -O2
    )
    add_link_options(-fsanitize=address)

    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
      message(WARNING "! You are using Adrress Sanitizer in non-Debug mode, which may lead to unexpected behavior and performance issues. !")
    endif()
  endif()
endif()
# =========================== ENABLE ADDRESS SANITIZER ===========================