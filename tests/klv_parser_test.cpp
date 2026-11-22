#include "klv_parser.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace
{

constexpr uint8_t ST0601_KEY[16] = {0x06, 0x0E, 0x2B, 0x34, 0x02, 0x0B, 0x01, 0x01,
                                    0x0E, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00};

void pushItem(std::vector<uint8_t>& body, std::vector<uint8_t> tag, std::vector<uint8_t> value)
{
  body.insert(body.end(), tag.begin(), tag.end());
  if (value.size() < 0x80)
  {
    body.push_back(static_cast<uint8_t>(value.size()));
  }
  else
  {
    body.push_back(0x81);
    body.push_back(static_cast<uint8_t>(value.size()));
  }
  body.insert(body.end(), value.begin(), value.end());
}

/// Key + BER length + body + tag 1 checksum, per ST 0601.
std::vector<uint8_t> finishPacket(const uint8_t* key, std::vector<uint8_t> body)
{
  std::vector<uint8_t> out(key, key + 16);
  const std::size_t len = body.size() + 4;
  if (len < 0x80)
  {
    out.push_back(static_cast<uint8_t>(len));
  }
  else
  {
    out.push_back(0x81);
    out.push_back(static_cast<uint8_t>(len));
  }
  out.insert(out.end(), body.begin(), body.end());
  out.push_back(0x01);
  out.push_back(0x02);
  uint16_t bcc = 0;
  for (std::size_t i = 0; i < out.size(); ++i)
  {
    bcc = static_cast<uint16_t>(bcc + (out[i] << (8 * ((i + 1) % 2))));
  }
  out.push_back(static_cast<uint8_t>(bcc >> 8));
  out.push_back(static_cast<uint8_t>(bcc & 0xFF));
  return out;
}

/// Timestamp, a 130-byte call sign (tag 59), an extension tag 130 (BER-OID), then lat/lon.
std::vector<uint8_t> poseBody(bool long_items)
{
  std::vector<uint8_t> body;
  pushItem(body, {0x02}, {0x00, 0x04, 0x59, 0xF4, 0xA6, 0xAA, 0x4A, 0xA8});
  if (long_items)
  {
    pushItem(body, {59}, std::vector<uint8_t>(130, 'A'));
    pushItem(body, {0x81, 0x02}, {0x00});
  }
  pushItem(body, {13}, {0x55, 0x95, 0xB6, 0x6D});
  pushItem(body, {14}, {0x5B, 0x53, 0x60, 0xC4});
  return body;
}

TEST(KlvParser, DecodesValidPacket)
{
  const auto pkt = finishPacket(ST0601_KEY, poseBody(false));
  const auto rec = rain_aero::parseKlvPacket(pkt, 0);
  EXPECT_TRUE(rec.validity.isUsable());
  ASSERT_TRUE(rec.aircraft.latitude_deg.has_value());
}

TEST(KlvParser, RejectsChecksumMismatch)
{
  auto pkt = finishPacket(ST0601_KEY, poseBody(false));
  pkt[pkt.size() - 8] ^= 0x01;  // corrupt one byte of longitude
  const auto rec = rain_aero::parseKlvPacket(pkt, 0);
  EXPECT_FALSE(rec.validity.timestamp_present);
  EXPECT_FALSE(rec.aircraft.longitude_deg.has_value());
}

TEST(KlvParser, RejectsWrongUniversalLabel)
{
  uint8_t other_key[16] = {};
  const auto pkt = finishPacket(other_key, poseBody(false));
  const auto rec = rain_aero::parseKlvPacket(pkt, 0);
  EXPECT_FALSE(rec.validity.timestamp_present);
  EXPECT_FALSE(rec.aircraft.latitude_deg.has_value());
}

TEST(KlvParser, HandlesBerLongLengthAndBerOidTag)
{
  const auto pkt = finishPacket(ST0601_KEY, poseBody(true));
  const auto rec = rain_aero::parseKlvPacket(pkt, 0);
  EXPECT_TRUE(rec.validity.timestamp_present);
  ASSERT_TRUE(rec.aircraft.latitude_deg.has_value());
  ASSERT_TRUE(rec.aircraft.longitude_deg.has_value());
  EXPECT_EQ(rec.validity.tags_skipped, 3);  // tag 59, tag 130, tag 1
}

}  // namespace
