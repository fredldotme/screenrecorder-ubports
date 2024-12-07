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

#include "mp4.h"
#include <array>
#include <string>
#include <stdexcept>

#include <QDebug>
#include <QAudioDeviceInfo>

#define MINIMP4_IMPLEMENTATION
#include "../minimp4.h"

QAudioFormat audioFormatCheck();
inline static MP4E_track_t setupAudioTrack()
{
    MP4E_track_t ret;
    QAudioFormat format = audioFormatCheck();

    ret.object_type_indication = MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3;
    ret.track_media_kind = e_audio;
    ret.time_scale = format.sampleRate();
    ret.default_duration = 0;
    ret.u.a.channelcount = format.channelCount();

    return ret;
}

MuxMp4::MuxMp4(QObject* parent) : QObject(parent), m_audioTrack{setupAudioTrack()}
{

}

MuxMp4::~MuxMp4()
{
    stop();
}

static int write_callback(int64_t offset, const void *buffer, size_t size, void *token)
{
    qDebug() << "writing to file" << size;
    QFile *file = static_cast<QFile *>(token);
    file->seek(offset);
    return file->write((const char *)buffer, size) != size;
}

void MuxMp4::start(const QString fileName, const int width, const int height)
{
    m_file.setFileName(fileName);
    m_file.open(QIODevice::WriteOnly);
    m_mux = MP4E_open(0, 0, (void *)&m_file, write_callback);

    m_trackId = MP4E_add_track(m_mux, &m_audioTrack);

    qDebug() << "before mp4_h26x_write_init";

    if (MP4E_STATUS_OK != mp4_h26x_write_init(&m_mp4wr, m_mux, width, height, 0)) {
        qCritical() << "mp4_h26x_write_init failed";
        throw std::runtime_error("mp4_h26x_write_init failed");
    }

    qDebug() << "started MuxMp4";

    m_running = true;
}

static ssize_t get_nal_size(uint8_t *buf, ssize_t size)
{
    ssize_t pos = 3;
    while ((size - pos) > 3) {
        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 1)
            return pos;
        if (buf[pos] == 0 && buf[pos + 1] == 0 && buf[pos + 2] == 0 && buf[pos + 3] == 1)
            return pos;
        pos++;
    }
    return size;
}

// basically https://github.com/lieff/minimp4/blob/master/minimp4_test.c#L278-L300 - CC0
void MuxMp4::addBuffer(const Buffer::Ptr &buffer, const bool hasCodecConfig)
{
    qDebug() << "MuxMp4 got buffer";
    uint8_t *bufH264 = buffer->Data();
    uint32_t h264Size = buffer->Length();

    while (h264Size > 0) {
        ssize_t nalSize = get_nal_size(bufH264, h264Size);

        if (nalSize < 4) {
            bufH264 += 1;
            h264Size -= 1;
            continue;
        }

        if (MP4E_STATUS_OK != mp4_h26x_write_nal(&m_mp4wr, bufH264, nalSize, 90000 / 30)) {
            qCritical() << "mp4_h26x_write_nal failed";
        }

        bufH264 += nalSize;
        h264Size -= nalSize;
    }
}

void MuxMp4::addAudioBuffer(const Buffer::Ptr &buffer)
{
    MP4E_put_sample(m_mux, m_trackId, buffer->Data(), buffer->Length(), buffer->Timestamp(), MP4E_SAMPLE_DEFAULT);
}

void MuxMp4::stop()
{
    if (!m_running) {
        qWarning() << "trying to stop mp4 muxer that is not running";
        return;
    }

    MP4E_close(m_mux);
    mp4_h26x_write_close(&m_mp4wr);
    m_file.close();
    m_running = false;

    qDebug() << "stopped MuxMp4";
}

QAudioFormat audioFormatCheck()
{
    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleSize(8);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
    if (!info.isFormatSupported(format)) {
        qWarning() << "Default format not supported, trying to use the nearest.";
        format = info.nearestFormat(format);
    }

    return format;
}

QAudioFormat MuxMp4::audioFormat()
{
    return audioFormatCheck();
}
