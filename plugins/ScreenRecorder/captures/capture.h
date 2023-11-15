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

#ifndef CAPTURES_CAPTURE_H
#define CAPTURES_CAPTURE_H

#include <QtPlugin>
#include <QSharedPointer>
#include <QSemaphore>
#include "../buffer.h"

class Capture
{
public:
    virtual void setSemaphore(const QSharedPointer<QSemaphore> semaphore) = 0;
Q_SIGNALS:
    virtual void started(int width, int height, double framerate) = 0;
    virtual void bufferAvailable(const Buffer::Ptr &buffer) = 0;
public Q_SLOTS:
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void swapBuffers() = 0;
};

Q_DECLARE_INTERFACE(Capture, "screenrecorder.ubports.Capture")

#endif // CAPTURES_CAPTURE_H
