find_package(fmt CONFIG REQUIRED)
find_package(ffmpeg CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

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
