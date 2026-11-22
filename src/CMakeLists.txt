find_package(fmt CONFIG REQUIRED)
find_package(ffmpeg CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# misb: single-header MISB ST 0601 KLV decoder (UL, checksum, BER-OID tags).
include(FetchContent)
FetchContent_Declare(
  misb
  URL https://raw.githubusercontent.com/Mattbusel/misb/v0.4.0/cpp/misb.hpp
  URL_HASH SHA256=bfdd22c1583e70b6cef7ca2a76f3900dffe00c06cebd8df6ab4d849de6ea4210
  DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(misb)

add_library(greeter STATIC greeter.cpp greeter.h)
target_include_directories(
  greeter PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(
  greeter
  PRIVATE fmt::fmt
          quickstart_project_options
)

add_library(sensor_pipeline STATIC
  pose_types.h
  klv_parser.h
  klv_parser.cpp
  ts_reader.h
  ts_reader.cpp
  output_writer.h
  output_writer.cpp
)
target_include_directories(
  sensor_pipeline PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_include_directories(sensor_pipeline SYSTEM PRIVATE ${misb_SOURCE_DIR})
target_link_libraries(
  sensor_pipeline
  PUBLIC fmt::fmt
         ffmpeg::ffmpeg
         nlohmann_json::nlohmann_json
         quickstart_project_options
)

add_executable(quickstart main.cpp)
target_link_libraries(
  quickstart
  PRIVATE sensor_pipeline
          quickstart_project_options
)
