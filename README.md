# Rain Aero — Video Sensor Ingestion and Pose Export

A C++20 command-line application that ingests MPEG-TS video with embedded KLV metadata and emits a structured JSON pose record for each metadata sample. Designed for safety-critical aerial systems where data validity and deterministic output matter.

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| FFmpeg | 6.1 | MPEG-TS demux, KLV stream extraction |
| nlohmann_json | 3.11.3 | JSON serialization |
| fmt | 10.2.1 | String formatting |
| GTest | 1.15.0 | Unit testing |

All dependencies are managed via Conan 2. No system-level installs required beyond the devcontainer.

## Build Instructions

Requires the provided devcontainer (Ubuntu 24.04, Clang 18, Conan 2).

Open the project in VS Code and select **Reopen in Container** when prompted. Then in the integrated terminal:

```bash
./build.sh --clean
```

## Run

```bash
./build/debug/src/quickstart <input.ts> <output.json>
```

Example:

```bash
./build/debug/src/quickstart S1001073.TS sample_output/output.json
```

Expected terminal output:
Reading KLV packets from 'S1001073.TS'...

Parsed 3131 records.

Usable: 3131  Unusable: 0

Writing output to 'sample_output/output.json'...

Done.

## Run Tests

```bash
./build.sh --test
```

## Output Format

A JSON array of pose records, one per KLV packet. Each record contains:

- `stream_pts` — raw MPEG-TS PTS value in 90 kHz ticks
- `timestamp_us` — MISP microseconds since Unix epoch
- `timestamp_iso` — UTC timestamp in ISO 8601 format
- `aircraft` — platform pose: latitude, longitude, altitude, heading, pitch, roll
- `camera` — sensor pose: latitude, longitude, altitude, azimuth, elevation, roll
- `validity` — diagnostic report: tags parsed, skipped, invalid, and usable flag

Optional fields serialize as `null` when absent in the KLV stream.

## Known Limitations

- Implements 11 of approximately 100 MISB ST 0601 tags: 2, 5, 6, 7, 13, 14, 15, 18, 19, 20, 70
- Camera altitude (tag 70) not present in sample file — serializes as null
- KLV packets are the unit of output, not individual decoded video frames
- Single-threaded; designed for offline processing not real-time ingestion
- Tested on Ubuntu 24.04 inside the provided devcontainer only
