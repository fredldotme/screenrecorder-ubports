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

#ifndef CAPTURES_MIR_H
#define CAPTURES_MIR_H

#include <QObject>
#include "capture.h"

#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/mir_screencast.h>
#include <mir_toolkit/mir_buffer_stream.h>

class CaptureMir : public QObject, public Capture
{
    Q_OBJECT
    Q_INTERFACES(Capture)
public:
    using QObject::QObject;
    ~CaptureMir();
    void setSemaphore(const QSharedPointer<QSemaphore> semaphore) override;
Q_SIGNALS:
    void started(int width, int height, double framerate) override;
    void bufferAvailable(const Buffer::Ptr &buffer) override;
public Q_SLOTS:
    void start() override;
    void stop() override;
    void swapBuffers() override;

private:
    MirConnection *m_connection = nullptr;
    MirScreencast *m_screencast = nullptr;
    MirBufferStream *m_bufferStream = nullptr;
    QSharedPointer<QSemaphore> m_semaphore;
};

#endif // CAPTURES_MIR_H
