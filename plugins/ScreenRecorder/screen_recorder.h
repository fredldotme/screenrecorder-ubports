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

#ifndef SCREEN_RECORDER_H
#define SCREEN_RECORDER_H

#include "captures/capture.h"
#include "encoders/encoder.h"
#include "muxers/mux.h"
#include "indicator.h"
#include "aacconverter.h"
#include <QObject>
#include <QThread>
#include <QSharedPointer>
#include <QTimer>
#include <QElapsedTimer>
#include <QAudioInput>
#include <QIODevice>

class ScreenRecorder : public QObject
{
    Q_OBJECT
public:
    ScreenRecorder(QObject *parent = nullptr);
    void setup(QSharedPointer<QObject> encoder, QSharedPointer<QObject> capture,
               QSharedPointer<QObject> mux);
public Q_SLOTS:
    void start(float framerate, bool mic);
    void stop();
    void bufferAvailable();
    void tick();

private:
    QThread m_captureThread;
    QThread m_encoderThread;
    QThread m_muxThread;
    QThread m_audioThread;
    QThread m_indicatorThread;
    QSharedPointer<QObject> m_encoder;
    QSharedPointer<QObject> m_capture;
    QSharedPointer<QObject> m_mux;
    QSharedPointer<QAudioInput> m_audioInput;
    QSharedPointer<QIODevice> m_microphoneAudio;
    AacConverter m_aacConverter;
    QTimer m_timer;
    QSharedPointer<Indicator> m_indicator;
    QElapsedTimer m_elapsed;
    uint64_t m_frames;
    bool m_mic;
};

#endif // SCREEN_RECORDER_H
