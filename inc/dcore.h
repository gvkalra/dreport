#pragma once

#include <glib.h>
#include <gio/gio.h>

typedef struct {
	gchar *key;
	guint32 value;
} dbus_stats_data_item;

typedef struct {
	const gchar *sender_name;
	const gchar *object_path;
	const gchar *interface_name;
	const gchar *signal_name;
	const gchar *message; /* Formatted message */
} dbus_listener_data_item;

typedef void (*dbus_listener_cb)(GDBusConnection *connection, dbus_listener_data_item *data, void *user_data);
typedef void (*dbus_listener_destroy_cb)(void *user_data);

GDBusConnection *
dbus_setup_connection(GBusType bus_type);

void
dbus_close_connection(GDBusConnection *connection);

gchar **
dbus_get_names(GDBusConnection *connection, gboolean allow_anonymous);

gchar **
dbus_get_object_paths(GDBusConnection *connection, const gchar *service);

GSList *
dbus_get_stats_summary(GDBusConnection *connection);

gint
dbus_register_listener(GDBusConnection *connection, dbus_listener_cb s_cb, void *user_data, dbus_listener_destroy_cb d_cb);

void
dbus_deregister_listener(GDBusConnection *connection, gint id);
