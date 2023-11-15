/*
 * Copyright (C) 2016 Canonical, Ltd.
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

#ifndef AC_VIDEO_BUFFERQUEUE_H_
#define AC_VIDEO_BUFFERQUEUE_H_

#include <QObject>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "buffer.h"

class BufferQueue : public QObject
{
    Q_OBJECT
public:
    BufferQueue(QObject *parent = nullptr, uint32_t max_size = 0);

    Buffer::Ptr next();
    Buffer::Ptr front();

    void lock();
    void unlock();

    void push(const Buffer::Ptr &buffer);
    void pushUnlocked(const Buffer::Ptr &buffer);

    Buffer::Ptr pop();
    Buffer::Ptr popUnlocked();

    bool waitForSlots(const std::chrono::milliseconds &timeout = std::chrono::milliseconds{ 1 });
    bool waitToBeFilled(const std::chrono::milliseconds &timeout = std::chrono::milliseconds{ 1 });

    bool isLimited() const { return m_max_size != 0; }
    bool isFull();
    bool isEmpty();

    int size();

    bool waitFor(const std::function<bool()> &pred, const std::chrono::milliseconds &timeout);

private:
    uint32_t m_max_size;
    std::queue<Buffer::Ptr> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_lock;
};

#endif
