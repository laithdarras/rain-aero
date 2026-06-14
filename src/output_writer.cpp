/// output_writer.cpp - JSON serialization implementation.
///
/// Uses nlohmann_json to serialize FrameRecord values to a formatted JSON array
/// Optional fields are written as null when absent so downstream consumers can distinguish "not present" from zero

#include "output_writer.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "pose_types.h"

namespace rain_aero
{

  namespace
  {

    /// Format a MispTimestamp as an ISO 8601 UTC string.
    /// Example: "2025-05-26T14:30:47.123456Z"
    [[nodiscard]] std::string formatTimestamp(const MispTimestamp& ts)
    {
      const auto us = ts.time_since_epoch();
      const auto s  = std::chrono::duration_cast<std::chrono::seconds>(us);
      const auto remaining_us =
          std::chrono::duration_cast<std::chrono::microseconds>(us) -
          std::chrono::duration_cast<std::chrono::microseconds>(s);

      const std::time_t time_t_val = s.count();
      std::tm           utc_tm{};
      gmtime_r(&time_t_val, &utc_tm);

      std::array<char, 32> buf{};
      std::strftime(buf.data(), buf.size(), "%Y-%m-%dT%H:%M:%S", &utc_tm);

      return std::string{buf.data()} +
             "." +
             std::to_string(remaining_us.count()) +
             "Z";
    }

    /// Serialize an optional double which writes the value or JSON null.
    [[nodiscard]] nlohmann::json optionalToJson(const std::optional<double>& val)
    {
      if (val.has_value())
      {
        return nlohmann::json{static_cast<double>(*val)};
      }
      return nlohmann::json{nullptr};
    }

    /// Serialize an AircraftPose to a JSON object
    [[nodiscard]] nlohmann::json aircraftToJson(const AircraftPose& ac)
    {
      nlohmann::json obj;
      obj["latitude_deg"]  = optionalToJson(ac.latitude_deg);
      obj["longitude_deg"] = optionalToJson(ac.longitude_deg);
      obj["altitude_m"]    = optionalToJson(ac.altitude_m);
      obj["heading_deg"]   = optionalToJson(ac.heading_deg);
      obj["pitch_deg"]     = optionalToJson(ac.pitch_deg);
      obj["roll_deg"]      = optionalToJson(ac.roll_deg);
      return obj;
    }

    /// Serialize a CameraPose to a JSON object
    [[nodiscard]] nlohmann::json cameraToJson(const CameraPose& cam)
    {
      nlohmann::json obj;
      obj["latitude_deg"]           = optionalToJson(cam.latitude_deg);
      obj["longitude_deg"]          = optionalToJson(cam.longitude_deg);
      obj["altitude_m"]             = optionalToJson(cam.altitude_m);
      obj["relative_azimuth_deg"]   = optionalToJson(cam.relative_azimuth_deg);
      obj["relative_elevation_deg"] = optionalToJson(cam.relative_elevation_deg);
      obj["relative_roll_deg"]      = optionalToJson(cam.relative_roll_deg);
      return obj;
    }

    /// Serialize a ValidityReport to a JSON object
    [[nodiscard]] nlohmann::json validityToJson(const ValidityReport& vr)
    {
      nlohmann::json obj;
      obj["timestamp_present"]      = vr.timestamp_present;
      obj["aircraft_pose_complete"] = vr.aircraft_pose_complete;
      obj["camera_pose_complete"]   = vr.camera_pose_complete;
      obj["tags_parsed"]            = vr.tags_parsed;
      obj["tags_skipped"]           = vr.tags_skipped;
      obj["tags_invalid"]           = vr.tags_invalid;
      obj["usable"]                 = vr.isUsable();
      return obj;
    }

    /// Serialize a single FrameRecord to a JSON object
    [[nodiscard]] nlohmann::json recordToJson(const FrameRecord& record)
    {
      nlohmann::json obj;

      obj["stream_pts"] = record.stream_pts;

      // Write timestamp in two formats: raw microseconds and ISO 8601.
      if (record.timestamp.has_value())
      {
        const auto us = record.timestamp->time_since_epoch().count();
        obj["timestamp_us"]  = us;
        obj["timestamp_iso"] = formatTimestamp(*record.timestamp);
      }
      else
      {
        obj["timestamp_us"]  = nullptr;
        obj["timestamp_iso"] = nullptr;
      }

      obj["aircraft"] = aircraftToJson(record.aircraft);
      obj["camera"]   = cameraToJson(record.camera);
      obj["validity"] = validityToJson(record.validity);

      return obj;
    }

  }  // namespace

  bool writeJson(const std::vector<FrameRecord>& records,
                 const std::filesystem::path&    out_path)
  {
    nlohmann::json output = nlohmann::json::array();

    for (const FrameRecord& record : records)
    {
      output.push_back(recordToJson(record));
    }

    std::ofstream file{out_path};
    if (!file.is_open())
    {
      return false;
    }

    file << output.dump(2) << "\n";
    return file.good();
  }

}  // namespace rain_aero
