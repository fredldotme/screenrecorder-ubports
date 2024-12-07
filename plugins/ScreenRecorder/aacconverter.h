#ifndef AACCONVERTER_H
#define AACCONVERTER_H

extern "C" {

#include <libavcodec/avcodec.h>

class AacConverter {
public:
AacConverter() : audioCodec{nullptr}, codec{nullptr} {};
AacConverter(const int sampleRate, const int channels) {
    avcodec_register_all();

    // Set up audio encoder
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (codec == NULL)
        return;

    audioCodec = avcodec_alloc_context3(codec);
    audioCodec->bit_rate = 128000;
    audioCodec->sample_fmt = AV_SAMPLE_FMT_S16;
    audioCodec->sample_rate = sampleRate;
    audioCodec->channels = channels;
    audioCodec->profile = FF_PROFILE_AAC_MAIN;
    audioCodec->time_base = (AVRational){1, sampleRate};
    audioCodec->codec_type = AVMEDIA_TYPE_AUDIO;
    avcodec_open2(audioCodec, codec, nullptr);
};
~AacConverter() {
    if (audioCodec) {
        avcodec_close(audioCodec);
        av_free(audioCodec);
        audioCodec = nullptr;
    }
};

unsigned char* encodeWav(const char* data, unsigned int length, unsigned int& bufSize)
{
    frameEncode = av_frame_alloc();

    int rawOffset = 0;
    int rawDelta = 0;
    int rawSamplesCount = frameEncode->nb_samples <= length ? frameEncode->nb_samples : length;
    char* dataPtr = (char*)data;

    while (rawSamplesCount > 0)
    {
        memcpy(frameEncode->data[0], &dataPtr[rawOffset], sizeof(uint8_t) * rawSamplesCount);

        encodeFrame();

        rawOffset += rawSamplesCount;
        rawDelta = length - rawOffset;
        rawSamplesCount = rawDelta > frameEncode->nb_samples ? frameEncode->nb_samples : rawDelta;
    }

    av_frame_unref(frameEncode);

    bufSize = collectedSamples.size();
    return collectedSamples.data();
}

void encodeFrame()
{
    /* send the frame for encoding */
    int ret = avcodec_send_frame(audioCodec, frameEncode);
    if (ret < 0)
    {
        return;
    }

    /* read all the available output packets (in general there may be any number of them) */
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(audioCodec, &packetEncode);
        if (ret < 0 && ret != AVERROR(EAGAIN)) continue;
        if (ret < 0) break;
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
    AVCodecContext *audioCodec;
    AVCodec *codec;
    AVPacket packetEncode;
    AVFrame* frameEncode;
    std::vector<uint8_t> collectedSamples;
};

}

#endif
