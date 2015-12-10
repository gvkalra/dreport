#pragma once

#include <glib.h>
#include <gio/gio.h>

GDBusConnection *
dbus_setup_connection(GBusType bus_type);

void
dbus_close_connection(GDBusConnection *connection);

gchar **
dbus_get_names(GDBusConnection *connection, gboolean allow_anonymous);

gchar **
dbus_get_object_paths(GDBusConnection *connection, const gchar *service);
