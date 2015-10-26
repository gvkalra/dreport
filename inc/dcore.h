#pragma once

#include <glib.h>
#include <gio/gio.h>
#include "dreport.h"

GDBusProxy *
dbus_setup_proxy(GBusType bus_type);

void
dbus_close_proxy(GDBusProxy *proxy);

gchar **
dbus_get_names(GDBusProxy *proxy);

gchar **
dbus_get_activatable_names(GDBusProxy *proxy);
