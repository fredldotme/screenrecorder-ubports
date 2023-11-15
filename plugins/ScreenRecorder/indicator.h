/*
 * Copyright 2014 Canonical Ltd.
 * Copyright 2022-2023 Robert Tari
 * Copyright 2023 Maciej Sopyło
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

#ifndef INDICATOR_H
#define INDICATOR_H

#include <gio/gio.h>
#include <QObject>
#include <QTime>
#include <memory>

#define SERVICE_NAME "ubports.screenrecorder.indicator"
#define SERVICE_PATH "/ubports/screenrecorder/indicator"
#define ACTIONS_PATH "/ubports/screenrecorder/indicator/actions"

#define INDICATOR_PATH \
    "/home/phablet/.local/share/ayatana/indicators/ubports.screenrecorder.indicator"

class Indicator : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    void onBusAqcuired(GDBusConnection *connection, const gchar *name);
public Q_SLOTS:
    void start();
    void stop();
    void updateElapsed(const QTime elapsed);
Q_SIGNALS:
    void stopped();

private:
    void enable();
    void disable();

    bool m_stopped = false;
    guint m_busId = 0;
    guint m_exportedActionsId = 0;
    guint m_exportedMenuId = 0;
    GDBusConnection *m_bus = nullptr;
    GSimpleActionGroup *m_action_group = nullptr;
    GMenuItem *m_elapsedItem = nullptr;
    GMenu *m_section = nullptr;
    std::shared_ptr<GIcon> m_icon = nullptr;
};

#endif // INDICATOR_H
