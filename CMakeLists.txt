cmake_minimum_required(VERSION 3.24)

project(
  cpp_quickstart
  LANGUAGES CXX
  VERSION 0.1.0
)

# ---------------------------------------------------------------------------
# Project-wide language and tooling defaults
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "No build type specified; defaulting to Debug")
  set(CMAKE_BUILD_TYPE
      "Debug"
      CACHE STRING "" FORCE
  )
endif()

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Sanitizers)
include(Warnings)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(BUILD_TESTS "Build unit tests" ON)

# ---------------------------------------------------------------------------
# Apply project-wide flags
# ---------------------------------------------------------------------------
add_library(quickstart_project_options INTERFACE)
quickstart_apply_warnings(quickstart_project_options)
quickstart_apply_sanitizers(quickstart_project_options)

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
add_subdirectory(src)

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
