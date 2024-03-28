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

#ifndef MUXERS_MP4_H
#define MUXERS_MP4_H

#include <QObject>
#include <QFile>
#include "../minimp4.h"
#include "mux.h"

class MuxMp4 : public QObject, public Mux
{
    Q_OBJECT
    Q_INTERFACES(Mux)
public:
    using QObject::QObject;
    ~MuxMp4();

Q_SIGNALS:
    void frameAppended(int64_t timestamp) override;

public Q_SLOTS:
    void addBuffer(const Buffer::Ptr &buffer, const bool hasCodecConfig) override;
    void start(const QString fileName, const int width, const int height) override;
    void stop() override;

private:
    bool m_running = false;
    QFile m_file;
    MP4E_mux_t *m_mux;
    mp4_h26x_writer_t m_mp4wr;
};

#endif // MUXERS_MP4_H
