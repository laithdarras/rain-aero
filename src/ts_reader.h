/// ts_reader.h - FFmpeg MPEG-TS demux wrapper
///
/// Opens an MPEG-TS file, locates the KLV metadata stream, and reads packets one at a time
/// Each KLV packet is passed through the KLV parser and accumulated into a vector of FrameRecords
///
/// This class owns the FFmpeg context and manages its lifetime via RAII.
/// The FFmpeg context is opened in the constructor and closed in the
/// destructor — no manual cleanup required by the caller.
///
/// Usage:
///   TsReader reader{"input.ts"};
///   if (!reader.isOpen()) { /* handle error */ }
///   auto records = reader.readAll();

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "pose_types.h"

// Forward-declare FFmpeg types to avoid pulling C headers into every
// translation unit that includes this header.
struct AVFormatContext;

namespace rain_aero
{

  class TsReader
  {
  public:

    /// Open the MPEG-TS file at the given path.
    /// On failure isOpen() returns false and errorMessage() explains why.
    explicit TsReader(const std::filesystem::path& path);

    /// Destructor closes the FFmpeg context and frees all resources.
    ~TsReader();

    // Non-copyable: FFmpeg contexts cannot be safely copied.
    TsReader(const TsReader&)            = delete;
    TsReader& operator=(const TsReader&) = delete;

    // Movable.
    TsReader(TsReader&&)            = default;
    TsReader& operator=(TsReader&&) = default;

    /// Returns true if the file was opened and a KLV stream was found.
    [[nodiscard]] bool isOpen() const;

    /// Returns a human-readable error message if isOpen() is false.
    [[nodiscard]] const std::string& errorMessage() const;

    /// Read all KLV packets from the file and return parsed FrameRecords.
    /// Returns an empty vector if isOpen() is false.
    [[nodiscard]] std::vector<FrameRecord> readAll();

  private:

    AVFormatContext* fmt_ctx_{nullptr};
    int              klv_stream_index_{-1};
    std::string      error_message_;

  };

}  // namespace rain_aero
