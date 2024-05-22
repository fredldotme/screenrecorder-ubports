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

#include "mir.h"

#include <QDebug>

namespace {
static constexpr const char *kMirSocket{ "/run/mir_socket" };
static constexpr const char *kMirConnectionName{ "screencapture client" };
} // namespace

CaptureMir::~CaptureMir()
{
    stop();

    if (m_connection) {
        mir_connection_release(m_connection);
    }
}

void CaptureMir::start()
{
    if (m_screencast || m_bufferStream) {
        qWarning() << "tried to start a capture while already started";
        return;
    }

    if (!m_connection) {
        m_connection = mir_connect_sync(kMirSocket, kMirConnectionName);
    }

    if (!mir_connection_is_valid(m_connection)) {
        qCritical() << "failed to connect to Mir server:"
                    << mir_connection_get_error_message(m_connection);
        return;
    }

    const auto config = mir_connection_create_display_config(m_connection);
    if (!config) {
        qCritical() << "failed to create display configuration:"
                    << mir_connection_get_error_message(m_connection);
        return;
    }

    MirDisplayOutput *activeOutput = nullptr;

    unsigned int usableOutputs = 0;
    for (uint8_t i = 0; i < config->num_outputs; ++i) {
        if (config->outputs[i].connected && config->outputs[i].used
            && config->outputs[i].current_mode < config->outputs[i].num_modes) {
            ++usableOutputs;
        }
    }

    for (uint8_t i = 0; i < config->num_outputs; ++i) {
        if (config->outputs[i].connected && config->outputs[i].used
            && config->outputs[i].current_mode < config->outputs[i].num_modes) {
            // Jump to the next possible external or virtual output if two monitors are detected.
            if (usableOutputs > 1 && i == 0)
                continue;

            // Found an active connection we can just use for our purpose
            activeOutput = &config->outputs[i];
            break;
        }
    }

    if (!activeOutput) {
        qCritical() << "failed to find a suitable display output";
        return;
    }

    const MirDisplayMode *displayMode = &activeOutput->modes[activeOutput->current_mode];

    auto spec = mir_create_screencast_spec(m_connection);
    if (!spec) {
        qCritical() << "failed to create Mir screencast specification:"
                    << mir_connection_get_error_message(m_connection);
        return;
    }

    mir_screencast_spec_set_width(spec, displayMode->horizontal_resolution);
    mir_screencast_spec_set_height(spec, displayMode->vertical_resolution);

    MirRectangle region;
    // If we request a screen region outside the available screen area
    // mir will create a mir output which is then available for everyone
    // as just another display.
    region.left = 0;
    region.top = 0;
    region.width = displayMode->horizontal_resolution;
    region.height = displayMode->vertical_resolution;

    mir_screencast_spec_set_capture_region(spec, &region);

    unsigned int numPixelFormats = 0;
    MirPixelFormat pixelFormat;
    mir_connection_get_available_surface_formats(m_connection, &pixelFormat, 1, &numPixelFormats);
    if (numPixelFormats == 0) {
        qCritical() << "failed to find suitable pixel format:"
                    << mir_connection_get_error_message(m_connection);
        return;
    }

    mir_screencast_spec_set_pixel_format(spec, pixelFormat);
    mir_screencast_spec_set_mirror_mode(spec, mir_mirror_mode_vertical);
    mir_screencast_spec_set_number_of_buffers(spec, 1);

    m_screencast = mir_screencast_create_sync(spec);
    mir_screencast_spec_release(spec);
    if (!mir_screencast_is_valid(m_screencast)) {
        qCritical() << "failed to create Mir screencast:"
                    << mir_screencast_get_error_message(m_screencast);
        return;
    }

    m_bufferStream = mir_screencast_get_buffer_stream(m_screencast);
    if (!m_bufferStream) {
        qCritical() << "failed to setup Mir buffer stream";
        return;
    }

    m_elapsed.restart();

    qDebug() << "started mir capture";
    Q_EMIT started(displayMode->horizontal_resolution, displayMode->vertical_resolution,
                   displayMode->refresh_rate);
}

void CaptureMir::stop()
{
    if (m_screencast) {
        mir_screencast_release_sync(m_screencast);
    }

    m_screencast = nullptr;
    m_bufferStream = nullptr;
    m_elapsed.invalidate();
}

void CaptureMir::swapBuffers()
{
    qDebug() << "swapping buffers";
    if (!m_bufferStream) {
        return;
    }
    mir_buffer_stream_swap_buffers_sync(m_bufferStream);
    MirNativeBuffer *buffer = nullptr;
    mir_buffer_stream_get_current_buffer(m_bufferStream, &buffer);

    const auto wrappedBuffer = Buffer::Create(reinterpret_cast<void *>(buffer));
    wrappedBuffer->SetTimestamp(m_elapsed.elapsed() * 1000);
    Q_EMIT bufferAvailable(wrappedBuffer);
}
