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

#ifndef HYBRIS_MEDIA_META_DATA_H
#define HYBRIS_MEDIA_META_DATA_H

#include <stdexcept>
#include <string>
#include <hybris/media/media_codec_source_layer.h>

#define PROP_STRING(name, key)         \
    void name(const std::string value) \
    {                                  \
        setString(key, value);         \
    }
#define PROP_INT32(name, key)      \
    void name(const int32_t value) \
    {                              \
        setInt32(key, value);      \
    }

class HybrisMediaMetaData
{
public:
    HybrisMediaMetaData()
    {
        m_meta_data = media_meta_data_create();
        if (!m_meta_data) {
            throw std::runtime_error("media metadata creation failed!");
        }
    }

    ~HybrisMediaMetaData() { media_meta_data_release(m_meta_data); }

    PROP_STRING(mime, MEDIA_META_DATA_KEY_MIME)
    PROP_INT32(colorFormat, MEDIA_META_DATA_KEY_COLOR_FORMAT)
    PROP_INT32(width, MEDIA_META_DATA_KEY_WIDTH)
    PROP_INT32(height, MEDIA_META_DATA_KEY_HEIGHT)
    PROP_INT32(stride, MEDIA_META_DATA_KEY_STRIDE)
    PROP_INT32(sliceHeight, MEDIA_META_DATA_KEY_SLICE_HEIGHT)
    PROP_INT32(framerate, MEDIA_META_DATA_KEY_FRAMERATE)

    /**
     * returns the underlaying MediaMetaDataWrapper pointer and
     * consumes this class to avoid touching it after ownership
     * is passed to the compat layer
     */
    MediaMetaDataWrapper *data()
    {
        m_consumed = true;
        return m_meta_data;
    }

private:
    void setString(int key, std::string value)
    {
        if (m_consumed) {
            throw std::logic_error("tried to set value after metadata was consumed");
        }
        media_meta_data_set_cstring(m_meta_data, media_meta_data_get_key_id(key), value.c_str());
    }
    void setInt32(int key, int32_t value)
    {
        if (m_consumed) {
            throw std::logic_error("tried to set value after metadata was consumed");
        }
        media_meta_data_set_int32(m_meta_data, media_meta_data_get_key_id(key), value);
    }

    bool m_consumed = false;
    MediaMetaDataWrapper *m_meta_data;
};

#endif // HYBRIS_MEDIA_META_DATA_H
