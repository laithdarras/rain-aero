/// output_writer.h - JSON serialization for FrameRecord output.
///
/// Converts parsed FrameRecord values into a structured JSON output file.
/// Each record becomes one entry in a top-level JSON array.
///
/// Output format example:
///   {
///     "stream_pts": 324000000,
///     "timestamp_us": 1748293847123456,
///     "timestamp_iso": "2025-05-26T14:30:47.123456Z",
///     "aircraft": {
///       "latitude_deg": 37.123456,
///       "longitude_deg": -119.123456,
///       "altitude_m": 1234.5,
///       "heading_deg": 180.0,
///       "pitch_deg": 2.5,
///       "roll_deg": -1.2
///     },
///     "camera": {
///       "latitude_deg": 37.123456,
///       "longitude_deg": -119.123456,
///       "altitude_m": 1234.5,
///       "relative_azimuth_deg": 45.0,
///       "relative_elevation_deg": -30.0,
///       "relative_roll_deg": 0.0
///     },
///     "validity": {
///       "timestamp_present": true,
///       "aircraft_pose_complete": true,
///       "camera_pose_complete": true,
///       "tags_parsed": 10,
///       "tags_skipped": 2,
///       "tags_invalid": 0,
///       "usable": true
///     }
///   }

#pragma once

#include <filesystem>
#include <vector>

#include "pose_types.h"

namespace rain_aero
{

  /// Writes a vector of FrameRecords to a JSON file at the given path.
  ///
  /// @param records   All parsed records from the pipeline.
  /// @param out_path  Destination file path. Created or overwritten.
  /// @returns         True on success, false if the file could not be written.
  [[nodiscard]] bool writeJson(const std::vector<FrameRecord>& records,
                               const std::filesystem::path&    out_path);

}  // namespace rain_aero
