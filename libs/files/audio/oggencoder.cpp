#include "oggencoder.hpp"

#include <QFile>
#include <QByteArray>
#include <cstring>
#include <vector>
#include <cmath>

#include "log/logger.hpp"

#include <vorbis/vorbisenc.h>

namespace
{

struct OggStreamState
{
    ogg_stream_state os;
    bool initialized = false;

    ~OggStreamState()
    {
        if (initialized)
        {
            ogg_stream_clear(&os);
        }
    }
};

}

bool OggEncoder::encode(const QVector<float>& samples, const QString& outPath,
                          int sampleRate, int channels, int quality)
{
    if (samples.isEmpty())
    {
        LOG_ERROR("OggEncoder: empty sample buffer");
        return false;
    }
    if (channels < 1 || channels > 2)
    {
        LOG_ERROR(QString("OggEncoder: unsupported channel count %1 (only 1 or 2 supported)").arg(channels));
        return false;
    }
    if (sampleRate <= 0)
    {
        LOG_ERROR(QString("OggEncoder: invalid sample rate %1").arg(sampleRate));
        return false;
    }
    if (quality < -1 || quality > 10)
    {
        LOG_ERROR(QString("OggEncoder: invalid quality %1 (valid range -1 to 10, -1 = bitrate mode)").arg(quality));
        return false;
    }

    QFile outFile(outPath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        LOG_ERROR(QString("OggEncoder: cannot open output file %1: %2").arg(outPath, outFile.errorString()));
        return false;
    }

    vorbis_info info;
    vorbis_info_init(&info);

    int ret;
    if (quality < 0)
    {
        ret = vorbis_encode_init(&info, channels, sampleRate, -1, 128000, -1);
    }
    else
    {
        const double q = quality / 10.0;
        ret = vorbis_encode_init_vbr(&info, channels, sampleRate, q);
    }

    if (ret != 0)
    {
        LOG_ERROR(QString("OggEncoder: vorbis_encode_init failed (code %1)").arg(ret));
        vorbis_info_clear(&info);
        outFile.close();
        return false;
    }

    vorbis_comment comment;
    vorbis_comment_init(&comment);
    vorbis_comment_add_tag(&comment, "ENCODER", "OpenCK OggEncoder");

    vorbis_dsp_state dsp;
    if (vorbis_analysis_init(&dsp, &info) != 0)
    {
        LOG_ERROR("OggEncoder: vorbis_analysis_init failed");
        vorbis_comment_clear(&comment);
        vorbis_info_clear(&info);
        outFile.close();
        return false;
    }

    vorbis_block block;
    if (vorbis_block_init(&dsp, &block) != 0)
    {
        LOG_ERROR("OggEncoder: vorbis_block_init failed");
        vorbis_dsp_clear(&dsp);
        vorbis_comment_clear(&comment);
        vorbis_info_clear(&info);
        outFile.close();
        return false;
    }

    OggStreamState oos;
    if (ogg_stream_init(&oos.os, 0) != 0)
    {
        LOG_ERROR("OggEncoder: ogg_stream_init failed");
        vorbis_block_clear(&block);
        vorbis_dsp_clear(&dsp);
        vorbis_comment_clear(&comment);
        vorbis_info_clear(&info);
        outFile.close();
        return false;
    }
    oos.initialized = true;

    ogg_packet headerPacket;
    ogg_packet commentPacket;
    ogg_packet codebookPacket;
    if (vorbis_analysis_headerout(&dsp, &comment, &headerPacket, &commentPacket, &codebookPacket) != 0)
    {
        LOG_ERROR("OggEncoder: vorbis_analysis_headerout failed");
        vorbis_block_clear(&block);
        vorbis_dsp_clear(&dsp);
        vorbis_comment_clear(&comment);
        vorbis_info_clear(&info);
        outFile.close();
        return false;
    }

    ogg_stream_packetin(&oos.os, &headerPacket);
    ogg_stream_packetin(&oos.os, &commentPacket);
    ogg_stream_packetin(&oos.os, &codebookPacket);

    ogg_page page;
    while (ogg_stream_flush(&oos.os, &page) != 0)
    {
        outFile.write(reinterpret_cast<const char*>(page.header), page.header_len);
        outFile.write(reinterpret_cast<const char*>(page.body), page.body_len);
    }

    const int totalFrames = samples.size() / channels;
    const int chunkSize = 1024;
    int framesProcessed = 0;

    std::vector<float> interleaved;

    while (framesProcessed < totalFrames)
    {
        const int framesThisBlock = std::min(chunkSize, totalFrames - framesProcessed);

        float** buffer = vorbis_analysis_buffer(&dsp, framesThisBlock);
        if (!buffer)
        {
            LOG_ERROR("OggEncoder: vorbis_analysis_buffer returned null");
            vorbis_block_clear(&block);
            vorbis_dsp_clear(&dsp);
            vorbis_comment_clear(&comment);
            vorbis_info_clear(&info);
            outFile.close();
            return false;
        }

        for (int ch = 0; ch < channels; ++ch)
        {
            for (int i = 0; i < framesThisBlock; ++i)
            {
                buffer[ch][i] = samples[(framesProcessed + i) * channels + ch];
            }
        }

        if (vorbis_analysis_wrote(&dsp, framesThisBlock) != 0)
        {
            LOG_ERROR("OggEncoder: vorbis_analysis_wrote failed");
            vorbis_block_clear(&block);
            vorbis_dsp_clear(&dsp);
            vorbis_comment_clear(&comment);
            vorbis_info_clear(&info);
            outFile.close();
            return false;
        }

        framesProcessed += framesThisBlock;

        while (vorbis_analysis_blockout(&dsp, &block) == 1)
        {
            if (vorbis_analysis(&block, nullptr) != 0)
            {
                LOG_ERROR("OggEncoder: vorbis_analysis failed");
                vorbis_block_clear(&block);
                vorbis_dsp_clear(&dsp);
                vorbis_comment_clear(&comment);
                vorbis_info_clear(&info);
                outFile.close();
                return false;
            }

            if (vorbis_bitrate_addblock(&block) != 0)
            {
                LOG_ERROR("OggEncoder: vorbis_bitrate_addblock failed");
                vorbis_block_clear(&block);
                vorbis_dsp_clear(&dsp);
                vorbis_comment_clear(&comment);
                vorbis_info_clear(&info);
                outFile.close();
                return false;
            }

            ogg_packet op;
            while (vorbis_bitrate_flushpacket(&dsp, &op) == 1)
            {
                ogg_stream_packetin(&oos.os, &op);
                while (ogg_stream_pageout(&oos.os, &page) != 0)
                {
                    outFile.write(reinterpret_cast<const char*>(page.header), page.header_len);
                    outFile.write(reinterpret_cast<const char*>(page.body), page.body_len);
                }
            }
        }
    }

    if (vorbis_analysis_wrote(&dsp, 0) != 0)
    {
        LOG_WARNING("OggEncoder: final vorbis_analysis_wrote(0) reported non-zero");
    }

    ogg_stream_flush(&oos.os, &page);
    while (ogg_page_eos(&page) == 0 && ogg_stream_pageout(&oos.os, &page) != 0)
    {
        outFile.write(reinterpret_cast<const char*>(page.header), page.header_len);
        outFile.write(reinterpret_cast<const char*>(page.body), page.body_len);
    }

    outFile.close();

    vorbis_block_clear(&block);
    vorbis_dsp_clear(&dsp);
    vorbis_comment_clear(&comment);
    vorbis_info_clear(&info);

    LOG_INFO(QString("OggEncoder: wrote %1 (%2 frames, %3 Hz, %4 ch)")
                 .arg(outPath)
                 .arg(totalFrames)
                 .arg(sampleRate)
                 .arg(channels));
    return true;
}
