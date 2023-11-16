/*
 * Copyright (C) 2023  UBports Foundation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * screenrecorder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtQml>
#include <QtQml/QQmlContext>
#include <QMetaType>

#include "plugin.h"
#include "controller.h"
#include "buffer.h"

void ExamplePlugin::registerTypes(const char *uri)
{
    qRegisterMetaType<Buffer::Ptr>();
    //@uri Controller
    qmlRegisterSingletonType<Controller>(
            uri, 1, 0, "Controller",
            [](QQmlEngine *, QJSEngine *) -> QObject * { return new Controller; });
}
