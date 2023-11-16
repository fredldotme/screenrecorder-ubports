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

#include "bufferqueue.h"

BufferQueue::BufferQueue(QObject *parent, uint32_t max_size) : QObject(parent), m_max_size(max_size)
{
}

void BufferQueue::lock()
{
    m_mutex.lock();
}

void BufferQueue::pushUnlocked(const Buffer::Ptr &buffer)
{
    m_queue.push(buffer);
}

void BufferQueue::unlock()
{
    m_mutex.unlock();
    m_lock.notify_one();
}

Buffer::Ptr BufferQueue::front()
{
    std::unique_lock<std::mutex> l(m_mutex);
    return m_queue.front();
}

Buffer::Ptr BufferQueue::next()
{
    // We will block here forever until we get a new buffer but if
    // the wait call returns with false we're mostly likly terminating
    if (!waitToBeFilled(std::chrono::milliseconds{ -1 }))
        return nullptr;

    std::unique_lock<std::mutex> l(m_mutex);
    auto buffer = m_queue.front();
    m_queue.pop();
    return buffer;
}

void BufferQueue::push(const Buffer::Ptr &buffer)
{
    std::unique_lock<std::mutex> l(m_mutex);
    if (isLimited() && m_queue.size() >= m_max_size)
        return;
    m_queue.push(buffer);
    m_lock.notify_one();
}

Buffer::Ptr BufferQueue::pop()
{
    std::unique_lock<std::mutex> l(m_mutex);
    auto buffer = m_queue.front();
    m_queue.pop();
    m_lock.notify_one();
    return buffer;
}

Buffer::Ptr BufferQueue::popUnlocked()
{
    if (m_queue.size() == 0)
        return nullptr;

    auto buffer = m_queue.front();
    m_queue.pop();
    return buffer;
}

bool BufferQueue::waitFor(const std::function<bool()> &pred,
                          const std::chrono::milliseconds &timeout)
{
    std::unique_lock<std::mutex> l(m_mutex);

    if (!l.owns_lock())
        return false;

    if (timeout.count() >= 0) {
        auto now = std::chrono::system_clock::now();
        return m_lock.wait_until(l, now + timeout, pred);
    }

    m_lock.wait(l, pred);
    return true;
}

bool BufferQueue::waitToBeFilled(const std::chrono::milliseconds &timeout)
{
    if (isFull())
        return true;

    return waitFor([&]() { return !m_queue.empty(); }, timeout);
}

bool BufferQueue::waitForSlots(const std::chrono::milliseconds &timeout)
{
    if (!isLimited())
        return true;

    return waitFor([&]() { return m_queue.size() < m_max_size; }, timeout);
}

bool BufferQueue::isFull()
{
    if (!isLimited())
        return false;

    std::unique_lock<std::mutex> l(m_mutex);
    return m_queue.size() == m_max_size;
}

bool BufferQueue::isEmpty()
{
    std::unique_lock<std::mutex> l(m_mutex);
    return m_queue.size() == 0;
}

int BufferQueue::size()
{
    std::unique_lock<std::mutex> l(m_mutex);
    return m_queue.size();
}
