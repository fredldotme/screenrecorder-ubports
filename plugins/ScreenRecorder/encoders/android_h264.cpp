/*
 * Copyright (C) 2016 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "android_h264.h"

#include <system/window.h>

#include <QDebug>
#include <memory>
#include <stdexcept>

namespace {
static constexpr const char *kH264MimeType{ "video/avc" };
static constexpr const char *kRawMimeType{ "video/raw" };
// From frameworks/native/include/media/openmax/OMX_IVCommon.h
static constexpr int32_t kOMXColorFormatAndroidOpaque = 0x7F000789;
static constexpr int32_t kOMXVideoIntraRefreshCyclic = 0;
// From frameworks/native/include/media/openmax/OMX_Video.h
static constexpr int32_t kOMXVideoControlRateConstant = 2;
// Supplying -1 as framerate means the encoder decides on which framerate
// it provides.
static constexpr int32_t kAnyFramerate = 30;
// Default is a bitrate of 25 MBit/s
static constexpr int32_t kDefaultBitrate = 25000000;
// By default send an I frame every 15 seconds which is the
// same Android currently configures in its WiFi Display code path.
static constexpr std::chrono::seconds kDefaultIFrameInterval{ 15 };
// From frameworks/av/include/media/stagefright/MediaErrors.h
enum AndroidMediaError {
    kAndroidMediaErrorBase = -1000,
    kAndroidMediaErrorNotConnected = kAndroidMediaErrorBase - 1,
    kAndroidMediaErrorBufferTooSmall = kAndroidMediaErrorBase - 9,
    kAndroidMediaErrorEndOfStream = kAndroidMediaErrorBase - 11,
};

struct VideoNativeMetadata
{
    HybrisMediaMessage::MetadataBufferType eType; // must be ANWBuffer
    ANativeWindowBuffer *pBuffer;
    int nFenceFd; // -1 if unused
};
} // namespace

class MediaSourceBuffer : public Buffer
{
public:
    typedef std::shared_ptr<MediaSourceBuffer> Ptr;

    ~MediaSourceBuffer()
    {
        if (!buffer_)
            return;

        media_meta_data_release(meta_data);

        const auto ref_count = media_buffer_get_refcount(buffer_);

        // If someone has set a reference on the buffer we just have to
        // release it here and the other one will take care about actually
        // destroying it.
        if (ref_count > 0)
            media_buffer_release(buffer_);

        // Destroying the MediaBufferWrapper here assumes the underlying MediaBuffer(Base)
        // to be managed by some other entity, which is indeed the case for MediaCodecSource.
        media_buffer_destroy(buffer_);
    }

    static MediaSourceBuffer::Ptr Create(MediaBufferWrapper *buffer)
    {
        const auto sp = std::shared_ptr<MediaSourceBuffer>(new MediaSourceBuffer);
        sp->buffer_ = buffer;
        sp->meta_data = media_buffer_get_meta_data(buffer);
        ;
        sp->ExtractTimestamp();
        return sp;
    }

    virtual uint32_t Length() const { return media_buffer_get_size(buffer_); }

    virtual uint8_t *Data() { return static_cast<uint8_t *>(media_buffer_get_data(buffer_)); }

    virtual bool IsValid() const { return buffer_ != nullptr; }

private:
    void ExtractTimestamp()
    {
        if (!meta_data)
            return;

        const uint32_t key_time = media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME);
        int64_t time_us = 0;
        media_meta_data_find_int64(meta_data, key_time, &time_us);

        SetTimestamp(time_us);
    }

private:
    MediaBufferWrapper *buffer_;
    MediaMetaDataWrapper *meta_data;
};

AndroidH264Encoder::~AndroidH264Encoder()
{
    stop();

    if (m_encoder) {
        media_codec_source_release(m_encoder);
    }
}

void AndroidH264Encoder::configure(const Config &config)
{
    qDebug() << "configuring with" << config.width << "x" << config.height << "@"
             << config.output_scale;

    int width = static_cast<int>(static_cast<float>(config.width) * config.output_scale);
    int height = static_cast<int>(static_cast<float>(config.height) * config.output_scale);

    m_format = std::make_unique<HybrisMediaMessage>();
    m_format->mime(kH264MimeType);
    m_format->storeMetaDataInBuffers(HybrisMediaMessage::MetadataBufferType::ANWBuffer);
    m_format->storeMetaDataInBuffersAndroid(HybrisMediaMessage::MetadataBufferType::ANWBuffer);
    m_format->storeMetaDataInBuffersOutput(false);
    m_format->storeMetaDataInBuffersOutputAndroid(false);
    m_format->usingRecorder(true);
    m_format->width(width);
    m_format->height(height);
    m_format->stride(width);
    m_format->sliceHeight(height);
    m_format->colorFormat(kOMXColorFormatAndroidOpaque);
    // ↓ doesn't seem to work, on FP4 the resulting bitrate is ~8.7Mb/s
    m_format->bitrate(config.bitrate);
    m_format->bitrateMode(kOMXVideoControlRateConstant);
    m_format->framerate(config.framerate);
    //m_format->intraRefreshMode(kOMXVideoIntraRefreshCyclic);

    // Update macroblocks in a cyclic fashion with 10% of all MBs within
    // frame gets updated at one time. It takes about 10 frames to
    // completely update a whole video frame. If the frame rate is 30,
    // it takes about 333 ms in the best case (if next frame is not an IDR)
    // to recover from a lost/corrupted packet.
    //const int32_t mbs = (((config.width + 15) / 16) * ((config.height + 15) / 16) * 10) / 100;
    //m_format->intraRefreshCIRMbs(mbs);

    if (config.i_frame_interval > 0) {
        m_format->iFrameInterval(config.i_frame_interval);
    }
    if (config.profile_idc > 0) {
        m_format->profileIdc(config.profile_idc);
    }
    if (config.level_idc > 0) {
        m_format->levelIdc(config.level_idc);
    }
    if (config.constraint_set > 0) {
        m_format->constraintSet(config.constraint_set);
    }

    // FIXME we need to find a way to check if the encoder supports prepending
    // SPS/PPS to the buffers it is producing or if we have to manually do that
    m_format->prependSpsPpstoIdrFrames(true);

    m_sourceFormat = std::make_unique<HybrisMediaMetaData>();

    // Notice that we're passing video/raw as mime type here which is quite
    // important to let the encoder do the right thing with the incoming data
    m_sourceFormat->mime(kRawMimeType);

    // We're setting the opaque color format here as the encoder is then
    // meant to figure out the color format from the GL frames itself.
    m_sourceFormat->colorFormat(kOMXColorFormatAndroidOpaque);

    m_sourceFormat->width(width);
    m_sourceFormat->height(height);
    m_sourceFormat->stride(width);
    m_sourceFormat->sliceHeight(height);
    m_sourceFormat->framerate(config.framerate);

    auto source = media_source_create();
    if (!source) {
        throw std::runtime_error("failed to create media input source for encoder");
    }
    media_source_set_format(source, m_sourceFormat->data());

    media_source_set_start_callback(source, &AndroidH264Encoder::onSourceStart, this);
    media_source_set_stop_callback(source, &AndroidH264Encoder::onSourceStop, this);
    media_source_set_read_callback(source, &AndroidH264Encoder::onSourceRead, this);
    media_source_set_pause_callback(source, &AndroidH264Encoder::onSourcePause, this);

    // The MediaSource will be now owned by the MediaCodecSource wrapper
    // inside our compatibility layer. It will make sure its freed when
    // needed.
    m_encoder = media_codec_source_create(m_format->data(), source, 0);
    if (!m_encoder) {
        media_source_release(source);
        throw std::runtime_error("failed to create encoder instance");
    }

    qDebug() << "encoder configured succesfully";
}

AndroidH264Encoder::Config AndroidH264Encoder::defaultConfig()
{
    Config config;
    config.framerate = kAnyFramerate;
    config.bitrate = kDefaultBitrate;
    config.i_frame_interval = kDefaultIFrameInterval.count();
    config.intra_refresh_mode = 1;
    return config;
}

int AndroidH264Encoder::onSourceStart(MediaMetaDataWrapper *meta, void *user_data)
{
    qDebug() << "on source start";
    return 0;
}

int AndroidH264Encoder::onSourceStop(void *user_data)
{
    qDebug() << "on source stop";
    return 0;
}

int AndroidH264Encoder::onSourcePause(void *user_data)
{
    qDebug() << "on source pause";
    return 0;
}

int AndroidH264Encoder::onSourceRead(MediaBufferWrapper **buffer, void *user_data)
{
    qDebug() << "on source read";
    auto thiz = static_cast<AndroidH264Encoder *>(user_data);

    if (!thiz || !thiz->m_running) {
        return kAndroidMediaErrorNotConnected;
    }

    if (!buffer) {
        return kAndroidMediaErrorBufferTooSmall;
    }

    const auto inputBuffer = thiz->m_inputQueue.next();
    if (!inputBuffer) {
        return kAndroidMediaErrorEndOfStream;
    }

    const auto nextBuffer = thiz->packBuffer(inputBuffer, inputBuffer->Timestamp());

    if (!nextBuffer) {
        return kAndroidMediaErrorEndOfStream;
    }

    *buffer = nextBuffer;

    Q_EMIT thiz->beganFrame(inputBuffer->Timestamp());

    return 0;
}

void AndroidH264Encoder::onBufferReturned(MediaBufferWrapper *buffer, void *user_data)
{
    auto thiz = static_cast<AndroidH264Encoder *>(user_data);

    if (!thiz) {
        return;
    }

    // Find the right pending buffer matching the returned one
    auto iter = thiz->m_pendingBuffers.begin();
    for (; iter != thiz->m_pendingBuffers.end(); ++iter) {
        if (iter->mediaBuffer == buffer)
            break;
    }

    if (iter == thiz->m_pendingBuffers.end()) {
        qWarning() << "Didn't remember returned buffer!?";
        return;
    }

    // Unset observer to be able to call release on the MediaBuffer
    // and reduce its reference count. It has an internal check if
    // an observer is still set or not before it will actually release
    // itself.
    media_buffer_set_return_callback(iter->mediaBuffer, nullptr, nullptr);

    // Destroy the wrapper. Since this buffer is managed by us it will
    // be destroyed as intended in MediaBufferPrivate destructor.
    media_buffer_destroy(iter->mediaBuffer);

    auto buf = iter->buffer;
    thiz->m_pendingBuffers.erase(iter);

    // After we've cleaned up everything we can send the buffer
    // back to the producer which then can reuse it.
    if (buf) {
        buf->Release();
    }

    Q_EMIT thiz->bufferReturned();
}

MediaBufferWrapper *AndroidH264Encoder::packBuffer(const Buffer::Ptr &inputBuffer,
                                                   const int64_t &timestamp)
{
    if (!inputBuffer->NativeHandle()) {
        qWarning() << "Ignoring buffer without native handle";
        return nullptr;
    }

    const auto anwb = reinterpret_cast<ANativeWindowBuffer *>(inputBuffer->NativeHandle());

    // We let the media buffer allocate the memory here to let it keep
    // the ownership and release the memory once its destroyed.
    auto buffer = media_buffer_create(sizeof(VideoNativeMetadata));
    if (!buffer) {
        return nullptr;
    }

    VideoNativeMetadata *data = (VideoNativeMetadata *)media_buffer_get_data(buffer);

    memset(data, 0, sizeof(VideoNativeMetadata));
    data->eType = HybrisMediaMessage::MetadataBufferType::ANWBuffer;
    data->pBuffer = anwb;
    data->nFenceFd = -1;

    media_buffer_set_return_callback(buffer, &AndroidH264Encoder::onBufferReturned, this);

    // We need to put a reference on the buffer here if we want the
    // callback we set above being called.
    media_buffer_ref(buffer);

    // TODO: HybrisMetaData::create(buffer) ?
    auto meta = media_buffer_get_meta_data(buffer);
    const auto key_time = media_meta_data_get_key_id(MEDIA_META_DATA_KEY_TIME);
    media_meta_data_set_int64(meta, key_time, timestamp);
    media_meta_data_release(meta);

    m_pendingBuffers.push_back(BufferItem{ inputBuffer, buffer });

    return buffer;
}

bool AndroidH264Encoder::bufferHasCodecConfig(MediaBufferWrapper *buffer)
{
    // TODO: HybrisMetaData::create(buffer) ?
    auto meta_data = media_buffer_get_meta_data(buffer);
    if (!meta_data)
        return false;

    uint32_t key_is_codec_config = media_meta_data_get_key_id(MEDIA_META_DATA_KEY_IS_CODEC_CONFIG);
    int32_t is_codec_config = 0;
    media_meta_data_find_int32(meta_data, key_is_codec_config, &is_codec_config);
    media_meta_data_release(meta_data);
    return static_cast<bool>(is_codec_config);
}

void AndroidH264Encoder::sendIDRFrame()
{
    if (!m_encoder) {
        return;
    }

    media_codec_source_request_idr_frame(m_encoder);
}

void AndroidH264Encoder::start()
{
    if (!m_encoder || m_running) {
        return;
    }
    qDebug() << "encoder starting";

    Q_EMIT started();
}

void AndroidH264Encoder::stop()
{
    if (!m_encoder || !m_running) {
        return;
    }

    if (!media_codec_source_stop(m_encoder)) {
        return;
    }
    qDebug() << "encoder stopping";

    m_running = false;
    Q_EMIT stopped();
}

void AndroidH264Encoder::addBuffer(const Buffer::Ptr &buffer)
{
    m_inputQueue.push(buffer);
    Q_EMIT receivedInputBuffer(buffer->Timestamp());
    qDebug() << "encoder added buffer";

    if (!m_encoder || !m_running) {
        m_running = true;

        if (!media_codec_source_start(m_encoder)) {
            qCritical() << "failed to start encoder";
            m_running = false;
            return;
        }
    }

    MediaBufferWrapper *bufferWrapper = nullptr;
    if (!media_codec_source_read(m_encoder, &bufferWrapper)) {
        qCritical() << "failed to read a new buffer from encoder";
        return;
    }

    auto mbuf = MediaSourceBuffer::Create(bufferWrapper);

    Q_EMIT finishedFrame(mbuf->Timestamp());

    auto hasCodecConfig = AndroidH264Encoder::bufferHasCodecConfig(bufferWrapper);
    Q_EMIT bufferAvailable(mbuf, hasCodecConfig);
}
