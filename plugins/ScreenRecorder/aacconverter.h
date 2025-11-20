#ifndef AACCONVERTER_H
#define AACCONVERTER_H

extern "C" {

#include <libavcodec/avcodec.h>

class AacConverter {
public:
AacConverter() : ctx{nullptr}, codec{nullptr} {};
AacConverter(const int sampleRate, const int channels) {
    qDebug() << "Desired sample rate:" << sampleRate;

    // Set up audio encoder
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (codec == NULL) {
        qDebug() << "Failed to find AAC encoder";
        return;
    }

    int i = 0;
    while (true) {
        if (codec->supported_samplerates[i] == NULL)
            break;
        qDebug() << "Supported sample rate:" << codec->supported_samplerates[i];
        ++i;
    }

    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        qDebug() << "Failed to allocate context";
    }
    
    ctx->bit_rate = 128000;
    ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    ctx->sample_rate = sampleRate;
    ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    ctx->channels = av_get_channel_layout_nb_channels(ctx->channel_layout);
    ctx->profile = FF_PROFILE_AAC_MAIN;
    ctx->time_base = (AVRational){1, sampleRate};
    ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    int ret = avcodec_open2(ctx, codec, nullptr);
    if (ret < 0) {
        qDebug() << "Failed to open codec:" << ret;
    }
};
~AacConverter() {
    if (ctx) {
        avcodec_close(ctx);
        av_free(ctx);
        ctx = nullptr;
    }
};

unsigned char* encodeWav(const char* data, unsigned int length, unsigned int& bufSize)
{
    frameEncode = av_frame_alloc();

    if (!frameEncode)
        return nullptr;

    frameEncode->nb_samples = ctx->frame_size;
    frameEncode->format = ctx->sample_fmt;
    frameEncode->channel_layout = ctx->channel_layout;

    int rawOffset = 0;
    int rawDelta = 0;
    int rawSamplesCount = frameEncode->nb_samples <= length ? frameEncode->nb_samples : length;
    char* dataPtr = (char*)data;
    int i, j, k, ret;
    uint16_t* samples = nullptr;
    float t, tincr;

    qDebug() << "AAC rawSamplesCount" << rawSamplesCount <<
                "frameEncode->nb_samples" << frameEncode->nb_samples <<
                "length" << length;

    /* allocate the data buffers */
    ret = av_frame_get_buffer(frameEncode, 0);
    if (ret < 0) {
        qDebug() << "Could not allocate audio data buffers:" << ret;
        return nullptr;
    }

    ret = av_frame_make_writable(frameEncode);
    if (ret < 0) {
        qDebug() << "Failed to make frame writable:" << ret;
        return nullptr;
    }

    /*samples = (uint8_t*)frameEncode->data[0];
    if (!samples) {
        qDebug() << "Couldn't access frame data";
        return nullptr;
    }*/

    /*while (rawSamplesCount > 0)
    {
        memcpy(samples, &dataPtr[rawOffset], sizeof(uint8_t) * rawSamplesCount);
        rawOffset += rawSamplesCount;
        rawDelta = length - rawOffset;
        rawSamplesCount = rawDelta > frameEncode->nb_samples ? frameEncode->nb_samples : rawDelta;
    }*/
    
     /* encode a single tone sound */
     t = 0;
     tincr = 2 * M_PI * 440.0 / ctx->sample_rate;
     for (i = 0; i < 200; i++) {
         /* make sure the frame is writable -- makes a copy if the encoder
          * kept a reference internally */
         ret = av_frame_make_writable(frameEncode);
         if (ret < 0)
             exit(1);
         samples = (uint16_t*)frameEncode->data[0];
  
         for (j = 0; j < ctx->frame_size; j++) {
             samples[2*j] = (int)(sin(t) * 10000);
  
             for (k = 1; k < ctx->ch_layout.nb_channels; k++)
                 samples[2*j + k] = samples[2*j];
             t += tincr;
         }
         //encode(c, frame, pkt, f);
         encodeFrame();
     }

    qDebug() << "2" << samples << frameEncode->data[0] << &dataPtr[rawOffset];

    //encodeFrame();

    qDebug() << "3";

    av_frame_unref(frameEncode);

    bufSize = collectedSamples.size();
    return collectedSamples.data();
}

void encodeFrame()
{
    qDebug() << Q_FUNC_INFO;

    /* send the frame for encoding */
    int ret = avcodec_send_frame(ctx, frameEncode);
    if (ret < 0)
    {
        qDebug() << "avcodec_send_frame returned" << ret;
        return;
    }

    qDebug() << ret;

    /* read all the available output packets (in general there may be any number of them) */
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(ctx, &packetEncode);
        if (ret < 0 && ret != AVERROR(EAGAIN))
            continue;

        if (ret < 0)
            break;

        uint8_t* data = (uint8_t *)(malloc(sizeof(uint8_t) * packetEncode.size));
        memcpy(data, packetEncode.data, (size_t)packetEncode.size);
        const auto size = (unsigned int)(packetEncode.size);

        for (unsigned int i = 0; i < size; i++) {
            collectedSamples.push_back(data[i]);
        }
        free(data);
    }
    av_packet_unref(&packetEncode);
}

private:
    AVCodecContext *ctx;
    AVCodec *codec;
    AVPacket packetEncode;
    AVFrame* frameEncode;
    std::vector<uint8_t> collectedSamples;
};

}

#endif
