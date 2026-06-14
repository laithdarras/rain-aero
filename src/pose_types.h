/// pose_types.h - Core domain types for the Rain Aero sensor ingestion pipeline.
///
/// Defines the data structures that flow through the entire pipeline:
///
///   Raw KLV bytes -> AircraftPose / CameraPose -> FrameRecord -> JSON output
///
/// Design principles:
///   - std::optional for every field that may be absent or undecodable.
///   - Absent is NOT the same as zero (that matters within operational safety)
///   - All angles in degrees. All distances in meters.
///   - No FFmpeg, no JSON, no I/O. Pure domain types only.

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace rain_aero
{

  /// UTC timestamp from MISB ST 0601 tag 2 (Precision Timestamp)
  /// MISP epoch matches Unix epoch: 1970-01-01T00:00:00Z, in microseconds
  using MispTimestamp = std::chrono::time_point<std::chrono::system_clock,
                                                std::chrono::microseconds>;

  /// Aircraft platform pose from MISB ST 0601 platform attitude tags
  ///
  /// Fields map directly to ST 0601 tags:
  ///   heading_deg  → tag 5   range [0, 360)
  ///   pitch_deg    → tag 6   range [-20, 20]
  ///   roll_deg     → tag 7   range [-50, 50]
  ///   latitude_deg → tag 13  range [-90, 90]
  ///   longitude_deg→ tag 14  range [-180, 180]
  ///   altitude_m   → tag 15  MSL in meters
  struct AircraftPose
  {
    std::optional<double> latitude_deg;
    std::optional<double> longitude_deg;
    std::optional<double> altitude_m;
    std::optional<double> heading_deg;
    std::optional<double> pitch_deg;
    std::optional<double> roll_deg;
  };

  /// Camera sensor pose from MISB ST 0601 sensor pointing tags.
  ///
  ///   latitude_deg           → tag 13
  ///   longitude_deg          → tag 14
  ///   altitude_m             → tag 70  sensor ellipsoid height
  ///   relative_azimuth_deg   → tag 18  range [0, 360)
  ///   relative_elevation_deg → tag 19  range [-180, 180]
  ///   relative_roll_deg      → tag 20  range [-180, 180]
  struct CameraPose
  {
    std::optional<double> latitude_deg;
    std::optional<double> longitude_deg;
    std::optional<double> altitude_m;
    std::optional<double> relative_azimuth_deg;
    std::optional<double> relative_elevation_deg;
    std::optional<double> relative_roll_deg;
  };

  /// Diagnostic summary for one parsed KLV packet
  ///
  /// Carried alongside pose data so downstream consumers know exactly what was present/missing and what was out of range
  /// This is what makes the output useful in a safety-critical context since a consumer can reject/flag records that are incomplete
  struct ValidityReport
  {
    bool timestamp_present{false};
    bool aircraft_pose_complete{false};  // all 6 aircraft fields decoded
    bool camera_pose_complete{false};    // all 6 camera fields decoded
    int  tags_parsed{0};                 // tags successfully decoded
    int  tags_skipped{0};                // tags present but not implemented
    int  tags_invalid{0};                // tags with out-of-range values

    /// True if the record has the minimum viable data for consumption: a timestamp and a position fix.
    /// This is an extremely important safety-critical engineering design principle called Defensive Data Validation:
    /// Validate ALL the data inputs and ANYTHING that doesn't match the data contract gets rejected
    [[nodiscard]] bool isUsable() const
    {
      return timestamp_present &&
             tags_parsed > 0 &&
             tags_invalid == 0;
    }
  };

  /// One complete output record: a single KLV packet's parsed data is tied to a video frame by timestamp!
  ///
  /// This is the unit of output: one FrameRecord per KLV packet and then serialized to JSON by the output.
  struct FrameRecord
  {
    /// Raw PTS from the MPEG-TS stream in 90 kHz ticks.
    /// Divide by 90000 to get seconds.
    int64_t stream_pts{0};

    /// Wall-clock timestamp from KLV tag 2.
    /// absent if tag 2 was missing or malformed.
    std::optional<MispTimestamp> timestamp;

    /// Parsed platform pose.
    AircraftPose aircraft;

    /// Parsed camera/sensor pose.
    CameraPose camera;

    /// Validity and diagnostic information for this record.
    ValidityReport validity;
  };

}  // namespace rain_aero
