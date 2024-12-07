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

char* encodeWav(char* data, unsigned int length, unsigned int& bufSize)
{
    frameEncode = av_frame_alloc();

    int rawOffset = 0;
    int rawDelta = 0;
    int rawSamplesCount = frameEncode->nb_samples <= length ? frameEncode->nb_samples : length;

    while (rawSamplesCount > 0)
    {
        memcpy(frameEncode->data[0], &data[rawOffset], sizeof(uint8_t) * rawSamplesCount);

        encodeFrame();

        rawOffset += rawSamplesCount;
        rawDelta = length - rawOffset;
        rawSamplesCount = rawDelta > frameEncode->nb_samples ? frameEncode->nb_samples : rawDelta;
    }

    av_frame_unref(frameEncode);

    bufSize = 0;
    return nullptr;
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
        std::pair<uint8_t*, unsigned int> p = std::pair<uint8_t*, unsigned int>();
        p.first = (uint8_t *)(malloc(sizeof(uint8_t) * packetEncode.size));
        memcpy(p.first, packetEncode.data, (size_t)packetEncode.size);
        p.second = (unsigned int)(packetEncode.size);

        listEncode.push_back(p); // place encoded data into list to finally create one array of encoded data from it
    }
    av_packet_unref(&packetEncode);
}

#if 0
char* encodeWav(char* wav, int size, unsigned int& bufSize)
{
    int ret;
    char* buf;

    bufSize = 48000 * 8 * 1;
    buf = (char*)malloc(bufSize);
    if (buf == NULL) return nullptr;

    /* the codec gives us the frame size, in samples */
    frame_size = audioCodec->frame_size;
    samples = malloc(frame_size * 2 * audioCodec->channels);
    outbuf_size = 10000;
    outbuf = malloc(outbuf_size);

    /* encode a single tone sound */
    t = 0;
    tincr = 2 * M_PI * 440.0 / audioCodec->sample_rate;
    for(i=0;i<200;i++) {
        for(j=0;j<frame_size;j++) {
            samples[2*j] = (int)(sin(t) * 10000);
            samples[2*j+1] = samples[2*j];
            t += tincr;
        }
        /* encode the samples */
        out_size = avcodec_encode_audio2(audioCodec, outbuf, outbuf_size, samples);
        buf + 
    }
    free(outbuf);
    free(samples);

    return buf;
}
#endif

private:
    AVCodecContext *audioCodec;
    AVCodec *codec;
    AVPacket packetEncode;
    AVFrame* frameEncode;
    std::vector<std::pair<uint8_t*, unsigned int>> listEncode;
};

}

#endif
