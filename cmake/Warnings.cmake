include_guard(GLOBAL)

# Apply a warning baseline to the given INTERFACE target.
function(quickstart_apply_warnings target)
  target_compile_options(
    ${target} INTERFACE
      -Wall
      -Wextra
      -Wpedantic
      -Wcast-align
      -Wconversion
      -Wfloat-equal
      -Wshadow
      -Wundef
      -Wunreachable-code
      -Werror=return-type
      -Werror=switch
      -Werror=uninitialized
      -Werror=unused-result
      -Werror=infinite-recursion
  )
endfunction()
