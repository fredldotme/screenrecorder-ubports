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

#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <chrono>

#include "controller.h"
#include "buffer.h"

Controller::Controller()
{
    // make directory on launch so users can restart before starting a recording
    // TODO: remove once Lomiri does that itself
    QDir().mkpath(QFileInfo(INDICATOR_PATH).dir().absolutePath());
    m_capture = QSharedPointer<CaptureMir>(new CaptureMir());
    m_encoder = QSharedPointer<AndroidH264Encoder>(new AndroidH264Encoder());
    m_mux = QSharedPointer<MuxMp4>(new MuxMp4());
    m_recorder.setup(m_encoder, m_capture, m_mux);
}

Controller::~Controller() { }

void Controller::start(unsigned int width, unsigned int height, float scale, float framerate)
{
    auto config = AndroidH264Encoder::defaultConfig();
    config.width = width;
    config.height = height;
    config.output_scale = scale;
    m_encoder->configure(config);

    auto dir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);

    m_mux->start(dir.append("/screen_recording_")
                         .append(QDateTime::currentDateTime().toString("yyyy_MM_dd__hh_mm_ss_zzz"))
                         .append(".mp4"));
    m_recorder.start(framerate);
}

void Controller::stop()
{
    m_recorder.stop();
    m_mux->stop();
}
