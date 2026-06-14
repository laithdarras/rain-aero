#include "ts_reader.h"

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include "klv_parser.h"
#include "pose_types.h"

namespace rain_aero
{

  TsReader::TsReader(const std::filesystem::path& path)
  {
    const std::string path_str = path.string();

    if (avformat_open_input(&fmt_ctx_, path_str.c_str(), nullptr, nullptr) < 0)
    {
      error_message_ = "Could not open file: " + path_str;
      return;
    }

    if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0)
    {
      error_message_ = "Could not read stream info from: " + path_str;
      avformat_close_input(&fmt_ctx_);
      return;
    }

    for (unsigned int i = 0; i < fmt_ctx_->nb_streams; ++i)
    {
      const AVStream*   stream     = fmt_ctx_->streams[i];
      const AVMediaType media_type = stream->codecpar->codec_type;
      const AVCodecID   codec_id   = stream->codecpar->codec_id;

      if ((media_type == AVMEDIA_TYPE_DATA) &&
          (codec_id   == AV_CODEC_ID_SMPTE_KLV))
      {
        klv_stream_index_ = static_cast<int>(i);
        break;
      }
    }

    if (klv_stream_index_ < 0)
    {
      error_message_ = "No KLV stream found in: " + path_str;
      avformat_close_input(&fmt_ctx_);
      return;
    }
  }

  TsReader::~TsReader()
  {
    if (fmt_ctx_ != nullptr)
    {
      avformat_close_input(&fmt_ctx_);
    }
  }

  bool TsReader::isOpen() const
  {
    return fmt_ctx_ != nullptr && klv_stream_index_ >= 0;
  }

  const std::string& TsReader::errorMessage() const
  {
    return error_message_;
  }

  std::vector<FrameRecord> TsReader::readAll()
  {
    if (!isOpen()) { return {}; }

    std::vector<FrameRecord> records;
    records.reserve(4096);

    AVPacket* pkt = av_packet_alloc();
    if (pkt == nullptr) { return {}; }

    while (av_read_frame(fmt_ctx_, pkt) >= 0)
    {
      if (pkt->stream_index == klv_stream_index_)
      {
        const std::span<const uint8_t> data{
            pkt->data, static_cast<std::size_t>(pkt->size)};
        records.push_back(parseKlvPacket(data, pkt->pts));
      }
      av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    return records;
  }

}  // namespace rain_aero
