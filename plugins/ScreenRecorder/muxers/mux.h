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

#ifndef MUXERS_MUX_H
#define MUXERS_MUX_H

#include <QtPlugin>

#include <QAudioFormat>

#include "../buffer.h"

class Mux
{
Q_SIGNALS:
    virtual void frameAppended(int64_t timestamp) = 0;

public Q_SLOTS:
    virtual void addBuffer(const Buffer::Ptr &buffer, const bool hasCodecConfig) = 0;
    virtual void addAudioBuffer(const Buffer::Ptr &buffer) = 0;
    virtual QAudioFormat audioFormat() = 0;
    virtual void start(const QString fileName, const int width, const int height) = 0;
    virtual void stop() = 0;
};

Q_DECLARE_INTERFACE(Mux, "screenrecorder.ubports.Mux")

#endif // MUXERS_MUX_H
