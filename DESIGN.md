# Design Document — Video Sensor Ingestion and Pose Export

## Architecture Overview

The pipeline is divided into four independent layers:
MPEG-TS file

→ TsReader        (FFmpeg demux — extracts raw KLV packets)

→ parseKlvPacket  (MISB ST 0601 binary decoder — bytes to pose values)

→ FrameRecord     (domain type — validated, structured pose record)

→ writeJson       (nlohmann_json serializer — FrameRecord to JSON)

### Key Files

| File | Responsibility |
|------|---------------|
| `pose_types.h` | Core domain types: AircraftPose, CameraPose, ValidityReport, FrameRecord |
| `ts_reader.h/cpp` | FFmpeg RAII wrapper — owns AVFormatContext lifetime |
| `klv_parser.h/cpp` | MISB ST 0601 tag decoder — raw bytes to typed values |
| `output_writer.h/cpp` | JSON serialization — FrameRecord to output file |
| `main.cpp` | Entry point — wires pipeline, prints diagnostics |

### Design Principles

**Separation of concerns.** FFmpeg is isolated inside TsReader. Nothing else in the codebase includes FFmpeg headers. The KLV parser takes a raw `std::span<const uint8_t>` — it has no knowledge of how bytes were obtained.

**std::optional for every pose field.** Absent is not the same as zero. A missing latitude and a latitude of 0.0 are different states. Downstream consumers must explicitly handle absent fields — the compiler enforces this.

**Validity alongside data.** Every FrameRecord carries a ValidityReport. Consumers know exactly how many tags were parsed, skipped, and invalid without inferring from null fields.

**RAII resource management.** TsReader owns the AVFormatContext. The destructor calls avformat_close_input unconditionally. No manual cleanup required at any call site.

## Error Handling Strategy

Errors are handled at three levels:

**Container level** — TsReader constructor sets an error message and leaves fmt_ctx_ null if the file cannot be opened or has no KLV stream. Callers check isOpen() before proceeding.

**Packet level** — parseKlvPacket returns a FrameRecord with empty fields if the packet is malformed or too short. It never throws.

**Tag level** — Individual tag decoders increment tags_invalid if a decoded value falls outside the spec-defined range. The field is left as std::nullopt rather than storing a corrupt value.

The philosophy: fail loud at boundaries, fail safe at the field level. A record that cannot be trusted is marked unusable. A record with one bad tag has that field absent and tags_invalid incremented.

## Assumptions

- Input is a well-formed MPEG-TS file with a SMPTE KLV data stream
- KLV packets conform to MISB ST 0601 local set encoding
- BER length encoding uses short form or long form up to 4 bytes
- The 16-byte Universal Label is present at the start of every packet
- PTS values are in 90 kHz ticks as reported by ffprobe
- MISP timestamp epoch matches Unix epoch (microseconds since 1970-01-01)

## Known Limitations and Future Improvements

**Tag coverage.** Only 11 of approximately 100 ST 0601 tags are implemented. Adding a tag requires one entry in the St0601Tag enum and one case in dispatchTag — designed for easy extension.

**Camera altitude absent.** Tag 70 is not present in the sample file and serializes as null. A future improvement would fall back to tag 15 when tag 70 is absent.

**No frame association.** KLV packets are the unit of output. A future improvement would associate each record with the nearest video frame by PTS for frame-accurate pose export.

**Single-threaded.** The pipeline is synchronous. For real-time ingestion a producer-consumer model with a packet queue would be appropriate.

**No UL key validation.** The 16-byte Universal Label is skipped rather than verified. A production implementation would reject packets with an unexpected UL key.
