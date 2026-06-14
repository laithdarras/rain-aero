/// main.cpp — Rain Aero sensor ingestion pipeline entry point.
///
/// Usage: quickstart <input.ts> <output.json>

#include <cstdlib>
#include <filesystem>
#include <span>
#include <string_view>

#include <fmt/core.h>

#include "output_writer.h"
#include "pose_types.h"
#include "ts_reader.h"

int main(int argc, char** argv)
{
  const std::span<char* const> args{argv, static_cast<std::size_t>(argc)};

  if (args.size() < 3)
  {
    fmt::print(stderr, "Usage: quickstart <input.ts> <output.json>\n");
    return EXIT_FAILURE;
  }

  const std::filesystem::path input_path{args[1]};
  const std::filesystem::path output_path{args[2]};

  // --- Open and demux the MPEG-TS file ---
  rain_aero::TsReader reader{input_path};

  if (!reader.isOpen())
  {
    fmt::print(stderr, "Error: {}\n", reader.errorMessage());
    return EXIT_FAILURE;
  }

  fmt::print("Reading KLV packets from '{}'...\n", input_path.string());

  // --- Parse all KLV packets into FrameRecords ---
  const auto records = reader.readAll();

  fmt::print("Parsed {} records.\n", records.size());

  // --- Print a quick validity summary ---
  int usable   = 0;
  int unusable = 0;

  for (const auto& record : records)
  {
    if (record.validity.isUsable()) { ++usable; }
    else                            { ++unusable; }
  }

  fmt::print("Usable: {}  Unusable: {}\n", usable, unusable);

  // --- Write JSON output ---
  fmt::print("Writing output to '{}'...\n", output_path.string());

  if (!rain_aero::writeJson(records, output_path))
  {
    fmt::print(stderr, "Error: could not write output file '{}'\n",
               output_path.string());
    return EXIT_FAILURE;
  }

  fmt::print("Done.\n");
  return EXIT_SUCCESS;
}
