/*
 * Copyright (C) 2023  UBports Foundation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * screenrecorder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QPointer>
#include <memory>
#include "encoders/android_h264.h"
#include "captures/mir.h"
#include "muxers/mp4.h"
#include "screen_recorder.h"

class Controller : public QObject
{
    Q_OBJECT

public:
    Controller();
    ~Controller();

    Q_INVOKABLE void start(unsigned int width, unsigned int height, float scale, float framerate);
    Q_INVOKABLE void stop();

private:
    QSharedPointer<AndroidH264Encoder> m_encoder;
    QSharedPointer<CaptureMir> m_capture;
    QSharedPointer<MuxMp4> m_mux;
    ScreenRecorder m_recorder;
};

#endif // CONTROLLER_H
