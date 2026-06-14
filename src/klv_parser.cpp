/// klv_parser.cpp - MISB ST 0601 KLV packet parser implementation
///
/// Walks the tag-value pairs inside a KLV local set buffer and decodes
/// each recognized tag into the appropriate field of a FrameRecord.
///
/// Byte layout of a MISB ST 0601 local set packet:
///   [0..15]  16-byte Universal Label (UL key)
///   [16..17] BER-encoded length (short or long form)
///   [18..]   Tag-Value pairs: | tag (1 byte) | length (1 byte) | value (N bytes) |

#include "klv_parser.h"

#include <bit>
#include <chrono>
#include <cstdint>
#include <span>

#include "pose_types.h"

namespace rain_aero
{

  namespace
  {

    /// Number of bytes in the MISB ST 0601 Universal Label key.
    constexpr std::size_t UL_KEY_SIZE = 16;

    /// MISB ST 0601 tag identifiers for the fields we implement.
    /// Values match the tag numbers defined in ST 0601.17.
    enum class St0601Tag : uint8_t
    {
      PRECISION_TIMESTAMP       = 2,
      PLATFORM_HEADING          = 5,
      PLATFORM_PITCH            = 6,
      PLATFORM_ROLL             = 7,
      SENSOR_LATITUDE           = 13,
      SENSOR_LONGITUDE          = 14,
      SENSOR_TRUE_ALTITUDE      = 15,
      SENSOR_RELATIVE_AZIMUTH   = 18,
      SENSOR_RELATIVE_ELEVATION = 19,
      SENSOR_RELATIVE_ROLL      = 20,
      SENSOR_ELLIPSOID_HEIGHT   = 70,
    };

    /// Linear scaling: map a raw unsigned integer to a real-world range.
    /// Formula: real = (raw / max_raw) * (max_val - min_val) + min_val
    ///
    /// @param raw      Raw integer value read from the KLV bytes.
    /// @param max_raw  Maximum possible raw value for this field's byte width.
    /// @param min_val  Minimum real-world value (e.g. -90.0 for latitude).
    /// @param max_val  Maximum real-world value (e.g.  90.0 for latitude).
    [[nodiscard]] double linearMap(uint64_t raw,
                                   uint64_t max_raw,
                                   double   min_val,
                                   double   max_val)
    {
      return (static_cast<double>(raw) / static_cast<double>(max_raw)) *
             (max_val - min_val) + min_val;
    }

    /// Read N bytes from data (big-endian) into a uint64_t.
    /// KLV values are always big-endian per the MISB standard.
    ///
    /// @param data   Buffer to read from (needs to be at least n_bytes available)
    /// @param n_bytes # of bytes to read (1–8)
    [[nodiscard]] uint64_t readBigEndian(std::span<const uint8_t> data,
                                         std::size_t n_bytes)
    {
      uint64_t result = 0;
      for (std::size_t i = 0; i < n_bytes; ++i)
      {
        result = (result << 8U) | data[i];
      }
      return result;
    }

    /// Decode the BER-encoded length field and return the payload length
    /// Also advances the offset past the length bytes
    ///
    /// BER short form: 1 byte, value 0x00-0x7F = length directly.
    /// BER long form:  first byte = 0x80 | N, followed by N bytes of length.
    ///
    /// @param data    Full packet buffer.
    /// @param offset  Current position; advanced past the length bytes on return.
    /// @returns       Decoded length, or 0 if incorrectly structured.
    [[nodiscard]] std::size_t decodeBerLength(std::span<const uint8_t> data,
                                              std::size_t& offset)
    {
      if (offset >= data.size())
      {
        return 0;
      }

      const uint8_t first_byte = data[offset++];

      // Short form: high bit clear, value is the length directly.
      if ((first_byte & 0x80U) == 0)
      {
        return first_byte;
      }

      // Long form: low 7 bits = number of subsequent length bytes.
      const std::size_t n_len_bytes = first_byte & 0x7FU;

      if (n_len_bytes == 0 || n_len_bytes > 4 || offset + n_len_bytes > data.size())
      {
        return 0;  // malformed or unsupported length encoding
      }

      std::size_t length = 0;
      for (std::size_t i = 0; i < n_len_bytes; ++i)
      {
        length = (length << 8U) | data[offset++];
      }

      return length;
    }

    /// Decode tag 2: Precision Timestamp.
    /// 8-byte unsigned integer, MISP microseconds since Unix epoch.
    void decodeTimestamp(std::span<const uint8_t> value,
                         FrameRecord&             record)
    {
      if (value.size() != 8)
      {
        ++record.validity.tags_invalid;
        return;
      }

      const uint64_t raw_us = readBigEndian(value, 8);
      const auto duration = std::chrono::microseconds{raw_us};
      record.timestamp = MispTimestamp{duration};
      record.validity.timestamp_present = true;
    }

    /// Decode a 2-byte signed linear-mapped angle.
    /// Raw value is uint16; mapped linearly to [min_val, max_val].
    [[nodiscard]] std::optional<double> decode2ByteAngle(
        std::span<const uint8_t> value,
        double                   min_val,
        double                   max_val,
        ValidityReport&          validity)
    {
      if (value.size() != 2)
      {
        ++validity.tags_invalid;
        return std::nullopt;
      }

      constexpr uint64_t MAX_UINT16 = 0xFFFF;
      const uint64_t raw = readBigEndian(value, 2);
      const double scaled = linearMap(raw, MAX_UINT16, min_val, max_val);

      // Validate the decoded value is within the expected range.
      if (scaled < min_val || scaled > max_val)
      {
        ++validity.tags_invalid;
        return std::nullopt;
      }

      ++validity.tags_parsed;
      return scaled;
    }

    /// Decode a 4-byte signed linear-mapped value.
    /// Raw value is uint32; mapped linearly to [min_val, max_val].
    [[nodiscard]] std::optional<double> decode4ByteValue(
        std::span<const uint8_t> value,
        double                   min_val,
        double                   max_val,
        ValidityReport&          validity)
    {
      if (value.size() != 4)
      {
        ++validity.tags_invalid;
        return std::nullopt;
      }

      constexpr uint64_t MAX_UINT32 = 0xFFFF'FFFF;
      const uint64_t raw = readBigEndian(value, 4);
      const double scaled = linearMap(raw, MAX_UINT32, min_val, max_val);

      if (scaled < min_val || scaled > max_val)
      {
        ++validity.tags_invalid;
        return std::nullopt;
      }

      ++validity.tags_parsed;
      return scaled;
    }

    /// Dispatch a single decoded tag to the correct field in the record.
    void dispatchTag(St0601Tag                tag,
                     std::span<const uint8_t> value,
                     FrameRecord&             record)
    {
      AircraftPose&  ac  = record.aircraft;
      CameraPose&    cam = record.camera;
      ValidityReport& vr = record.validity;

      switch (tag)
      {
        case St0601Tag::PRECISION_TIMESTAMP:
          decodeTimestamp(value, record);
          break;

        // --- Platform attitude ---
        case St0601Tag::PLATFORM_HEADING:
          ac.heading_deg = decode2ByteAngle(value, 0.0, 360.0, vr);
          break;

        case St0601Tag::PLATFORM_PITCH:
          ac.pitch_deg = decode2ByteAngle(value, -20.0, 20.0, vr);
          break;

        case St0601Tag::PLATFORM_ROLL:
          ac.roll_deg = decode2ByteAngle(value, -50.0, 50.0, vr);
          break;

        // --- Sensor position ---
        case St0601Tag::SENSOR_LATITUDE:
          ac.latitude_deg = decode4ByteValue(value, -90.0, 90.0, vr);
          cam.latitude_deg = ac.latitude_deg;
          break;

        case St0601Tag::SENSOR_LONGITUDE:
          ac.longitude_deg = decode4ByteValue(value, -180.0, 180.0, vr);
          cam.longitude_deg = ac.longitude_deg;
          break;

        case St0601Tag::SENSOR_TRUE_ALTITUDE:
          ac.altitude_m = decode2ByteAngle(value, -900.0, 19000.0, vr);
          break;

        // --- Camera pointing ---
        case St0601Tag::SENSOR_RELATIVE_AZIMUTH:
          cam.relative_azimuth_deg = decode4ByteValue(value, 0.0, 360.0, vr);
          break;

        case St0601Tag::SENSOR_RELATIVE_ELEVATION:
          cam.relative_elevation_deg = decode4ByteValue(value, -180.0, 180.0, vr);
          break;

        case St0601Tag::SENSOR_RELATIVE_ROLL:
          cam.relative_roll_deg = decode4ByteValue(value, -180.0, 180.0, vr);
          break;

        case St0601Tag::SENSOR_ELLIPSOID_HEIGHT:
          cam.altitude_m = decode2ByteAngle(value, -900.0, 19000.0, vr);
          break;
      }
    }

  }  // namespace

  FrameRecord parseKlvPacket(std::span<const uint8_t> data, int64_t pts)
  {
    FrameRecord record;
    record.stream_pts = pts;

    // --- Skip the 16-byte Universal Label key ---
    // We trust FFmpeg to have identified this as a KLV packet already.
    // In production we would verify the UL matches the ST 0601 key.
    if (data.size() <= UL_KEY_SIZE)
    {
      return record;  // packet too short to contain any tags
    }

    std::size_t offset = UL_KEY_SIZE;

    // --- Decode the BER outer length ---
    const std::size_t payload_length = decodeBerLength(data, offset);
    if (payload_length == 0 || offset + payload_length > data.size())
    {
      return record;  // malformed length field
    }

    // --- Walk the tag-value pairs ---
    const std::size_t payload_end = offset + payload_length;

    while (offset < payload_end)
    {
      // Each item: 1 byte tag, 1 byte length, N bytes value.
      if (offset + 2 > payload_end)
      {
        break;  // not enough bytes for a complete tag-length pair
      }

      const auto raw_tag     = static_cast<uint8_t>(data[offset++]);
      const auto value_length = static_cast<std::size_t>(data[offset++]);

      if (offset + value_length > payload_end)
      {
        break;  // value extends past the declared payload boundary
      }

      const auto value = data.subspan(offset, value_length);
      offset += value_length;

      // Dispatch to the appropriate decoder if we recognize this tag.
      const auto tag = static_cast<St0601Tag>(raw_tag);

      switch (tag)
      {
        case St0601Tag::PRECISION_TIMESTAMP:
        case St0601Tag::PLATFORM_HEADING:
        case St0601Tag::PLATFORM_PITCH:
        case St0601Tag::PLATFORM_ROLL:
        case St0601Tag::SENSOR_LATITUDE:
        case St0601Tag::SENSOR_LONGITUDE:
        case St0601Tag::SENSOR_TRUE_ALTITUDE:
        case St0601Tag::SENSOR_RELATIVE_AZIMUTH:
        case St0601Tag::SENSOR_RELATIVE_ELEVATION:
        case St0601Tag::SENSOR_RELATIVE_ROLL:
        case St0601Tag::SENSOR_ELLIPSOID_HEIGHT:
          dispatchTag(tag, value, record);
          break;

        default:
          // Unknown or unimplemented tag — skip it but count it.
          ++record.validity.tags_skipped;
          break;
      }
    }

    // --- Populate completeness flags ---
    record.validity.aircraft_pose_complete =
        record.aircraft.latitude_deg.has_value() &&
        record.aircraft.longitude_deg.has_value() &&
        record.aircraft.altitude_m.has_value() &&
        record.aircraft.heading_deg.has_value() &&
        record.aircraft.pitch_deg.has_value() &&
        record.aircraft.roll_deg.has_value();

    record.validity.camera_pose_complete =
        record.camera.latitude_deg.has_value() &&
        record.camera.longitude_deg.has_value() &&
        record.camera.altitude_m.has_value() &&
        record.camera.relative_azimuth_deg.has_value() &&
        record.camera.relative_elevation_deg.has_value() &&
        record.camera.relative_roll_deg.has_value();

    return record;
  }

}  // namespace rain_aero
