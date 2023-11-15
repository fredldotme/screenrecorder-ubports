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

#ifndef AC_VIDEO_BUFFER_H_
#define AC_VIDEO_BUFFER_H_

#include <QObject>
#include <QMetaType>
#include <memory>

#include "non_copyable.h"

class BufferOutputTarget;

class Buffer : public std::enable_shared_from_this<Buffer>
{
public:
    typedef std::shared_ptr<Buffer> Ptr;

    class Delegate : public NonCopyable
    {
    public:
        virtual void OnBufferFinished(const Buffer::Ptr &buffer) = 0;
    };

    virtual ~Buffer();

    static Buffer::Ptr Create(uint32_t capacity = 0, int64_t timestamp = 0ll);
    static Buffer::Ptr Create(uint8_t *data, uint32_t length);
    static Buffer::Ptr Create(void *native_handle);

    void SetRange(uint32_t offset, uint32_t length);
    void SetTimestamp(int64_t timestamp);

    virtual uint32_t Capacity() const { return capacity_; }
    virtual uint32_t Offset() const { return offset_; }
    virtual uint32_t Length() const { return length_; }
    virtual uint8_t *Data() { return data_ + offset_; }
    // Timestamp of the buffer in micro-seconds
    virtual int64_t Timestamp() const { return timestamp_; }

    virtual bool IsValid() const { return data_ != nullptr || native_handle_ != nullptr; }

    virtual void *NativeHandle() const { return native_handle_; }

    void SetDelegate(const std::weak_ptr<Delegate> &delegate);

    void Release();

protected:
    Buffer();
    Buffer(int64_t timestamp);

    void Allocate(uint32_t size);

private:
    std::weak_ptr<Delegate> delegate_;
    uint32_t capacity_;
    uint32_t length_;
    uint32_t offset_;
    uint8_t *data_;
    int64_t timestamp_;
    void *native_handle_;

    friend class BufferOutputTarget;
};

Q_DECLARE_METATYPE(Buffer::Ptr)

#endif
