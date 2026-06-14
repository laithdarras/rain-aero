include_guard(GLOBAL)

# Sanitizer plumbing.
#
# Toggled via the cache options below. The build script (build.sh) maps its
# CLI flags onto these. asan+ubsan is the only useful combination on stock
# Linux; tsan must be run alone.
option(QUICKSTART_SANITIZE_ASAN  "Enable AddressSanitizer"           OFF)
option(QUICKSTART_SANITIZE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(QUICKSTART_SANITIZE_TSAN  "Enable ThreadSanitizer"            OFF)

function(quickstart_apply_sanitizers target)
  if(NOT (QUICKSTART_SANITIZE_ASAN
          OR QUICKSTART_SANITIZE_UBSAN
          OR QUICKSTART_SANITIZE_TSAN))
    return()
  endif()

  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|AppleClang|GNU)$")
    message(
      FATAL_ERROR
        "Sanitizers require Clang or GCC. Found: ${CMAKE_CXX_COMPILER_ID}"
    )
  endif()

  if(QUICKSTART_SANITIZE_TSAN AND QUICKSTART_SANITIZE_ASAN)
    message(
      FATAL_ERROR
        "ThreadSanitizer cannot be combined with AddressSanitizer. Pick one."
    )
  endif()

  set(_sanitizers)
  if(QUICKSTART_SANITIZE_ASAN)
    list(APPEND _sanitizers address)
    message(STATUS "Sanitizer enabled: address")
  endif()
  if(QUICKSTART_SANITIZE_UBSAN)
    list(APPEND _sanitizers undefined)
    message(STATUS "Sanitizer enabled: undefined")
  endif()
  if(QUICKSTART_SANITIZE_TSAN)
    list(APPEND _sanitizers thread)
    message(STATUS "Sanitizer enabled: thread")
  endif()

  string(REPLACE ";" "," _sanitize_arg "${_sanitizers}")

  target_compile_options(
    ${target} INTERFACE
      -fsanitize=${_sanitize_arg}
      -fno-omit-frame-pointer
      -fno-optimize-sibling-calls
      -g
  )
  target_link_options(
    ${target} INTERFACE
      -fsanitize=${_sanitize_arg}
  )

  if(QUICKSTART_SANITIZE_ASAN)
    target_compile_definitions(${target} INTERFACE _GLIBCXX_ASSERTIONS)
  endif()
endfunction()
