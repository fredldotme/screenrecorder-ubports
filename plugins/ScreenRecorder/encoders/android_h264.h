#ifndef ENCODERS_ANDROID_H264_H
#define ENCODERS_ANDROID_H264_H

#include <QObject>
#include <QList>
#include <hybris/media/media_codec_source_layer.h>

#include "encoder.h"
#include "../hybris/media_message.h"
#include "../hybris/media_meta_data.h"
#include "../buffer.h"
#include "../bufferqueue.h"

class AndroidH264Encoder : public QObject, public Encoder
{
    Q_OBJECT
    Q_INTERFACES(Encoder)
public:
    class Config
    {
    public:
        Config()
            : width(0),
              height(0),
              output_scale(1.0f),
              bitrate(0),
              framerate(0),
              profile(0),
              level(0),
              profile_idc(0),
              level_idc(0),
              constraint_set(0),
              i_frame_interval(0)
        {
        }

        bool operator==(const Config &other) const
        {
            return width == other.width && height == other.height && bitrate == other.bitrate
                    && output_scale == other.output_scale && framerate == other.framerate
                    && profile == other.profile && level == other.level
                    && profile_idc == other.profile_idc && level_idc == other.level_idc
                    && constraint_set == other.constraint_set
                    && i_frame_interval == other.i_frame_interval
                    && intra_refresh_mode == other.intra_refresh_mode;
        }

        unsigned int width;
        unsigned int height;
        float output_scale;
        unsigned int bitrate;
        int framerate;
        // H.264 specifics
        unsigned int profile;
        unsigned int level;
        unsigned int profile_idc;
        unsigned int level_idc;
        unsigned int constraint_set;
        unsigned int i_frame_interval;
        unsigned int intra_refresh_mode;
    };

    using QObject::QObject;
    ~AndroidH264Encoder();
    void configure(const Config &config);
    bool isRunning() const { return m_running; }
    static AndroidH264Encoder::Config defaultConfig();

Q_SIGNALS:
    void bufferAvailable(const Buffer::Ptr &buffer, const bool hasCodecConfig) override;
    void bufferReturned() override;
    void started() override;
    void stopped() override;
    void beganFrame(int64_t timestamp) override;
    void finishedFrame(int64_t timestamp) override;
    void receivedInputBuffer(int64_t timestamp) override;

public Q_SLOTS:
    void sendIDRFrame();
    void start() override;
    void stop() override;
    void addBuffer(const Buffer::Ptr &buffer) override;

private:
    struct BufferItem
    {
        Buffer::Ptr buffer;
        MediaBufferWrapper *mediaBuffer;
    };

    std::unique_ptr<HybrisMediaMessage> m_format;
    std::unique_ptr<HybrisMediaMetaData> m_sourceFormat;
    MediaCodecSourceWrapper *m_encoder;
    QList<BufferItem> m_pendingBuffers;
    BufferQueue m_inputQueue;
    bool m_running = false;

    static int onSourceRead(MediaBufferWrapper **buffer, void *user_data);
    static int onSourceStart(MediaMetaDataWrapper *meta, void *user_data);
    static int onSourceStop(void *user_data);
    static int onSourcePause(void *user_data);
    static void onBufferReturned(MediaBufferWrapper *buffer, void *user_data);

    MediaBufferWrapper *packBuffer(const Buffer::Ptr &inputBuffer, const int64_t &timestamp);
    static bool bufferHasCodecConfig(MediaBufferWrapper *buffer);
};
#endif // ENCODERS_ANDROID_H264_H
