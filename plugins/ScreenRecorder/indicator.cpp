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

#include "indicator.h"

#include <QFile>
#include <QDir>
#include <QSettings>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
#include <unistd.h>
#include <lomiri-url-dispatcher.h>

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer gself)
{
    static_cast<Indicator *>(gself)->onBusAqcuired(connection, name);
}

static void on_name_lost(GDBusConnection *connection, const gchar *name, gpointer gself)
{
    qDebug() << "name lost";
}

static void on_stop_activated(GSimpleAction *a G_GNUC_UNUSED, GVariant *param G_GNUC_UNUSED,
                              gpointer gself)
{
    lomiri_url_dispatch_send("screenrecorderubports://stop", NULL, NULL);
}

void Indicator::start()
{
    m_stopped = false;
    qDebug() << "starting indicator";
    m_busId = g_bus_own_name(G_BUS_TYPE_SESSION, SERVICE_NAME, G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired, nullptr, on_name_lost, this, nullptr);
}

void Indicator::stop()
{
    if (m_bus != nullptr) {
        g_dbus_connection_unexport_menu_model(m_bus, m_exportedMenuId);
        g_dbus_connection_unexport_action_group(m_bus, m_exportedActionsId);
    }
    if (m_busId) {
        g_bus_unown_name(m_busId);
    }
    g_object_unref(m_section);
    g_object_unref(m_elapsedItem);
    g_clear_object(&m_bus);
    disable();
    m_stopped = true;
}

void Indicator::enable()
{
#ifdef CLICK_MODE
    // TODO: remove once Lomiri does that itself
    QDir().mkpath(QFileInfo(INDICATOR_PATH).dir().absolutePath());
    QSettings config(INDICATOR_PATH, QSettings::IniFormat);
    QString path = QString("%1_%2").arg(ACTIONS_PATH).arg(m_busId);
    config.beginGroup("Indicator Service");
    config.setValue("Name", "screenrecorder");
    config.setValue("ObjectPath", SERVICE_PATH);
    config.setValue("Position", 0);
    config.endGroup();
    config.beginGroup("phone");
    config.setValue("ObjectPath", path);
    config.endGroup();
    config.beginGroup("desktop");
    config.setValue("ObjectPath", path);
    config.endGroup();
    config.beginGroup("greeter");
    config.setValue("ObjectPath", path);
    config.endGroup();
#endif
}

void Indicator::disable()
{
#ifdef CLICK_MODE
    QFile config(INDICATOR_PATH);
    config.remove();
#endif
}

void Indicator::onBusAqcuired(GDBusConnection *connection, const gchar *name)
{
    enable();
    qDebug() << "bus aquired";
    m_bus = G_DBUS_CONNECTION(g_object_ref(G_OBJECT(connection)));

    m_action_group = g_simple_action_group_new();
    GSimpleAction *action = g_simple_action_new("screenrecorder", nullptr);
    g_signal_connect(action, "activate", G_CALLBACK(on_stop_activated), this);
    g_action_map_add_action(G_ACTION_MAP(m_action_group), G_ACTION(action));
    g_object_unref(G_OBJECT(action));

    GError *error = nullptr;
    guint id = g_dbus_connection_export_action_group(m_bus, SERVICE_PATH,
                                                     G_ACTION_GROUP(m_action_group), &error);
    if (id) {
        m_exportedActionsId = id;
        qDebug() << "exported actions:" << id;
    } else {
        qDebug() << "couldn't export action group to" << SERVICE_PATH << ":" << error->message;
    }
    g_clear_error(&error);

    auto icon = g_themed_icon_new("media-record");
    auto iconDeleter = [](GIcon *o) { g_object_unref(G_OBJECT(o)); };
    m_icon.reset(icon, iconDeleter);

    GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(b, "{sv}", "visible", g_variant_new_boolean(true));
    g_variant_builder_add(b, "{sv}", "title", g_variant_new_string("Recording"));
    // g_variant_builder_add(b, "{sv}", "label", g_variant_new_string("recording"));
    g_variant_builder_add(b, "{sv}", "accessible-desc",
                          g_variant_new_string("The screen is being recorded"));
    g_variant_builder_add(b, "{sv}", "icon", g_icon_serialize(m_icon.get()));

    GVariant *state = g_variant_builder_end(b);

    auto a = g_simple_action_new_stateful("actions-header", nullptr, state);
    g_action_map_add_action(G_ACTION_MAP(m_action_group), G_ACTION(a));

    GMenu *menu = g_menu_new();
    m_section = g_menu_new();

    m_elapsedItem = g_menu_item_new("Recording, 00:00:00", NULL);
    g_menu_append_item(m_section, m_elapsedItem);

    GMenuItem *itemSep = g_menu_item_new(NULL, NULL);
    g_menu_item_set_attribute(itemSep, "x-ayatana-type", "s", "org.ayatana.indicator.div");
    g_menu_append_item(m_section, itemSep);
    g_object_unref(itemSep);

    GMenuItem *item = g_menu_item_new("Stop Recording", "indicator.screenrecorder");
    g_menu_item_set_attribute(item, "x-ayatana-type", "s", "org.ayatana.indicator.link");
    g_menu_append_item(m_section, item);
    g_menu_append_section(menu, NULL, G_MENU_MODEL(m_section));

    // g_object_unref(section);
    g_object_unref(item);

    GMenuItem *header = g_menu_item_new(nullptr, "indicator.actions-header");
    g_menu_item_set_attribute(header, "x-ayatana-type", "s", "org.ayatana.indicator.root");
    g_menu_item_set_submenu(header, G_MENU_MODEL(menu));

    auto mainMenu = g_menu_new();
    g_menu_append_item(mainMenu, header);
    g_object_unref(header);

    QString path = QString("%1_%2").arg(ACTIONS_PATH).arg(m_busId);
    id = g_dbus_connection_export_menu_model(m_bus, path.toStdString().c_str(),
                                             G_MENU_MODEL(mainMenu), &error);
    if (id) {
        m_exportedMenuId = id;
        qDebug() << "exported menu:" << id;
    } else {
        qDebug() << "cannot export" << path << ":" << error->message;
    }
    g_clear_error(&error);
}

void Indicator::updateElapsed(const QTime elapsed)
{
    if (m_elapsedItem == nullptr || m_section == nullptr) {
        return;
    }

    auto timeStr = QString("Recording, %1").arg(elapsed.toString("HH:mm:ss"));
    GMenuItem *itemElapsed = g_menu_item_new(timeStr.toStdString().c_str(), NULL);

    g_menu_remove(m_section, 0);
    g_menu_insert_item(m_section, 0, itemElapsed);

    g_object_unref(itemElapsed);
}
