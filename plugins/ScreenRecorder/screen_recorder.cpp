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

#include "screen_recorder.h"

#include <QDebug>
#include <QWindow>
#include <QGuiApplication>
#include "./captures/mir.h"
#include "./encoders/encoder.h"
#include "./muxers/mux.h"
#include "./muxers/mp4.h"

ScreenRecorder::ScreenRecorder(QObject *parent) : QObject(parent), m_mic{false}
{
}

void ScreenRecorder::setup(QSharedPointer<QObject> encoder, QSharedPointer<QObject> capture,
                           QSharedPointer<QObject> mux)
{
    if (!encoder || !capture || !mux) {
        qCritical() << "passed null pointers to encoder, capture or mux";
        return;
    }
    if (!qobject_cast<Encoder *>(encoder.data())) {
        qCritical() << "encoder is not an instance of Encoder";
        return;
    }
    if (!qobject_cast<Capture *>(capture.data())) {
        qCritical() << "capture is not an instance of Capture";
        return;
    }
    if (!qobject_cast<Mux *>(mux.data())) {
        qCritical() << "mux is not an instance of Mux";
        return;
    }

    // Encoder and capture mechanism
    m_encoder = encoder;
    m_capture = capture;
    m_mux = mux;

    // Indicator
    m_indicator = QSharedPointer<Indicator>(new Indicator());

    m_encoder->moveToThread(&m_encoderThread);
    m_capture->moveToThread(&m_captureThread);
    m_mux->moveToThread(&m_muxThread);
    m_indicator->moveToThread(&m_indicatorThread);

    m_timer.setInterval(1000 / 60);

    // Video encode
    connect(m_capture.data(), SIGNAL(bufferAvailable(const Buffer::Ptr)), m_encoder.data(),
            SLOT(addBuffer(const Buffer::Ptr)));
    connect(m_encoder.data(), SIGNAL(bufferAvailable(const Buffer::Ptr, const bool)), m_mux.data(),
            SLOT(addBuffer(const Buffer::Ptr, const bool)));
    connect(m_encoder.data(), SIGNAL(bufferReturned()), this, SLOT(bufferAvailable()));
    connect(&m_timer, SIGNAL(timeout()), m_capture.data(), SLOT(swapBuffers()));
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));

    m_encoderThread.start();
    m_captureThread.start();
    m_muxThread.start();
    m_indicatorThread.start();
}

void ScreenRecorder::bufferAvailable()
{
    qDebug() << "buffer returned";
}

void ScreenRecorder::start(float framerate, bool mic)
{
#if 0
    m_mic = mic;
    if (mic) {
        // Setup
        const QAudioFormat format = static_cast<MuxMp4*>(m_mux.data())->audioFormat();
        m_aacConverter = AacConverter(format.sampleRate(), format.channelCount());

        // Audio capture
        const QAudioDeviceInfo audioDevice(QAudioDeviceInfo::defaultInputDevice());
        m_audioInput = QSharedPointer<QAudioInput>(new QAudioInput(audioDevice, format, this));
        m_audioInput->moveToThread(&m_audioThread);
        m_audioInput->setNotifyInterval(100);
        connect(m_audioInput.data(), &QAudioInput::stateChanged, this,
            [=](QAudio::State state){
                qDebug() << "QAudioInput state changed:" << state;
            }
        );
        connect(m_audioInput.data(), &QAudioInput::notify, this,
            [=](){
                qDebug() << "Reading microphone";
                const auto readBytes = m_microphoneAudio->readAll();
                qDebug() << "Read" << readBytes.size() << "bytes";
                // m_mux->addAudioBuffer(Buffer:Create(readBytes.constData(), readBytes.size()))
                unsigned int bufSize;
                uint8_t* aacBuf = (uint8_t*)m_aacConverter.encodeWav(readBytes.constData(), readBytes.size(), bufSize);

                qDebug() << "AAC buffer" << bufSize;
                if (bufSize > 0) {
                    static_cast<MuxMp4*>(m_mux.data())->addAudioBuffer(Buffer::Create(aacBuf, bufSize));
                }
            }
        );
        m_microphoneAudio = QSharedPointer<QIODevice>(m_audioInput->start());
    }
#endif

    m_frames = 0;
    m_timer.setInterval(static_cast<int>(1000.0f / framerate));
    m_elapsed.start();
    m_indicator->start();
    QMetaObject::invokeMethod(m_encoder.data(), "start", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_capture.data(), "start", Qt::QueuedConnection);
    m_timer.start();
#if 0
    if (mic)
        m_audioInput->resume();
#endif
}

void ScreenRecorder::stop()
{
#if 0
    if (m_mic)
        m_audioInput->stop();
#endif
    m_indicator->stop();
    m_timer.stop();
    m_elapsed.invalidate();
    qobject_cast<Capture *>(m_capture.data())->stop();
    qobject_cast<Encoder *>(m_encoder.data())->stop();
}

void ScreenRecorder::tick()
{
    m_frames += 1;
    if (m_frames % 60 == 0) {
        qDebug() << "tick";
        m_indicator->updateElapsed(QTime::fromMSecsSinceStartOfDay(m_elapsed.elapsed()));
    }
}
