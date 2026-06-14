/// klv_parser.h - MISB ST 0601 KLV packet parser
///
/// Decodes raw KLV bytes extracted from an MPEG-TS data stream into structured FrameRecord values
/// Implements a subset of MISB ST 0601 tags sufficient to extract aircraft pose and camera pose
///
/// Implemented tags:
///   Tag  2 — Precision Timestamp      (8 bytes, MISP microseconds)
///   Tag  5 — Platform Heading Angle   (2 bytes, mapped to [0, 360))
///   Tag  6 — Platform Pitch Angle     (2 bytes, mapped to [-20, 20])
///   Tag  7 — Platform Roll Angle      (2 bytes, mapped to [-50, 50])
///   Tag 13 — Sensor Latitude          (4 bytes, mapped to [-90, 90])
///   Tag 14 — Sensor Longitude         (4 bytes, mapped to [-180, 180])
///   Tag 15 — Sensor True Altitude     (2 bytes, mapped to [-900, 19000] m)
///   Tag 18 — Sensor Relative Azimuth  (4 bytes, mapped to [0, 360))
///   Tag 19 — Sensor Relative Elevation(4 bytes, mapped to [-180, 180])
///   Tag 20 — Sensor Relative Roll     (4 bytes, mapped to [-180, 180])
///   Tag 70 — Sensor Ellipsoid Height  (2 bytes, mapped to [-900, 19000] m)
///
/// All other tags are counted as skipped but do not cause parse failure
///
/// Used this for refernece: MISB ST 0601.17 - UAS Datalink Local Set

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "pose_types.h"

namespace rain_aero
{

  /// Parses a single raw KLV packet buffer into a FrameRecord
  ///
  /// @param data   Raw bytes of the KLV local set, as delivered by FFmpeg.
  ///               Must begin with the 16-byte MISB UL key followed by BER-encoded length and then the tag-value pairs
  /// @param pts    The MPEG-TS PTS value for this packet (90 kHz ticks).
  ///               Stored verbatim in the returned FrameRecord.
  /// @returns      A fully populated FrameRecord. Fields that could not be decoded are left as std::nullopt
  ///               The validity report reflects exactly what was and was not decoded
  [[nodiscard]] FrameRecord parseKlvPacket(std::span<const uint8_t> data,
                                           int64_t pts);

}  // namespace rain_aero
