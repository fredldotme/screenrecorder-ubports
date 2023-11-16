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

#ifndef ENCODERS_ENCODER_H
#define ENCODERS_ENCODER_H

#include <QtPlugin>
#include <cstdint>
#include "../buffer.h"

class Encoder
{
Q_SIGNALS:
    virtual void bufferAvailable(const Buffer::Ptr &buffer, const bool hasCodecConfig) = 0;
    virtual void bufferReturned() = 0;
    virtual void started() = 0;
    virtual void stopped() = 0;
    virtual void beganFrame(int64_t timestamp) = 0;
    virtual void finishedFrame(int64_t timestamp) = 0;
    virtual void receivedInputBuffer(int64_t timestamp) = 0;
public Q_SLOTS:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void addBuffer(const Buffer::Ptr &buffer) = 0;
};

Q_DECLARE_INTERFACE(Encoder, "screenrecorder.ubports.Encoder")

#endif // ENCODER_H
