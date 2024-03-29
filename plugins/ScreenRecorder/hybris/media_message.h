/*
 * Copyright (C) 2023 Maciej Sopyło
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

#ifndef HYBRIS_MEDIA_MESSAGE_H
#define HYBRIS_MEDIA_MESSAGE_H

#include <stdexcept>
#include <string>
#include <hybris/media/media_codec_source_layer.h>

#define PROP_STRING(name, key)         \
    void name(const std::string value) \
    {                                  \
        setString(key, value);         \
    }
#define PROP_INT32(name, key)      \
    void name(const int32_t value) \
    {                              \
        setInt32(key, value);      \
    }
#define PROP_ENUM(name, key, type) \
    void name(const type value)    \
    {                              \
        setInt32(key, value);      \
    }
#define PROP_BOOL(name, key)    \
    void name(const bool value) \
    {                           \
        setInt32(key, value);   \
    }

// TODO: maybe QObject and Q_PROPERTY?
class HybrisMediaMessage
{
public:
    enum MetadataBufferType {
        CameraSource = 0,
        GrallocSource = 1,
        ANWBuffer = 2,
        NativeHandleSource = 3,
        Invalid = -1,
    };

    HybrisMediaMessage()
    {
        m_message = media_message_create();
        if (!m_message) {
            throw std::runtime_error("media message creation failed!");
        }
    }

    ~HybrisMediaMessage() { media_message_release(m_message); }

    PROP_STRING(mime, "mime")
    PROP_ENUM(storeMetaDataInBuffers, "store-metadata-in-buffers", MetadataBufferType)
    PROP_ENUM(storeMetaDataInBuffersAndroid, "android._input-metadata-buffer-type",
              MetadataBufferType)
    PROP_BOOL(storeMetaDataInBuffersOutput, "store-metadata-in-buffers-output")
    PROP_BOOL(storeMetaDataInBuffersOutputAndroid, "android._store-metadata-in-buffers-output")
    PROP_BOOL(usingRecorder, "android._using-recorder")
    PROP_INT32(width, "width")
    PROP_INT32(height, "height")
    PROP_INT32(stride, "stride")
    PROP_INT32(sliceHeight, "slice-height")
    PROP_INT32(colorFormat, "color-format")
    PROP_INT32(bitrate, "bitrate")
    PROP_INT32(bitrateMode, "bitrate-mode")
    PROP_INT32(framerate, "frame-rate")
    PROP_INT32(intraRefreshMode, "intra-refresh-mode")
    PROP_INT32(intraRefreshCIRMbs, "intra-refresh-CIR-mbs")
    PROP_INT32(iFrameInterval, "i-frame-interval")
    PROP_INT32(profileIdc, "profile")
    PROP_INT32(levelIdc, "level")
    PROP_INT32(constraintSet, "constraint-set")
    PROP_BOOL(prependSpsPpstoIdrFrames, "prepend-sps-pps-to-idr-frames")

    /**
     * returns the underlaying MediaMessageWrapper pointer and
     * consumes this class to avoid touching it after ownership
     * is passed to the compat layer
     */
    MediaMessageWrapper *data()
    {
        m_consumed = true;
        return m_message;
    }

private:
    void setString(const char *key, std::string value)
    {
        if (m_consumed) {
            throw std::logic_error("tried to set value after message was consumed");
        }
        media_message_set_string(m_message, key, value.c_str(), 0);
    }
    void setInt32(const char *key, int32_t value)
    {
        if (m_consumed) {
            throw std::logic_error("tried to set value after message was consumed");
        }
        media_message_set_int32(m_message, key, value);
    }
    void setBool(const char *key, bool value)
    {
        if (m_consumed) {
            throw std::logic_error("tried to set value after message was consumed");
        }
        media_message_set_int32(m_message, key, (int32_t)value);
    }

    bool m_consumed = false;
    MediaMessageWrapper *m_message;
};

#endif // HYBRIS_MEDIA_MESSAGE_H
