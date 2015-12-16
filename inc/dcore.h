#pragma once

#include <glib.h>
#include <gio/gio.h>

typedef struct {
	gchar *key;
	guint32 value;
} dbus_stats_data_item;

typedef struct {
	guint type; /* GDBusMessageType */
	const gchar *sender;
	const gchar *destination;
	const gchar *message;
} dbus_listener_data_header;

typedef struct {
	dbus_listener_data_header hdr;
	const gchar *path;
	const gchar *interface;
	const gchar *member;
} dbus_listener_data_method;

typedef struct {
	dbus_listener_data_header hdr;
	const gchar *name;
} dbus_listener_data_error;

typedef union {
	dbus_listener_data_header hdr;
	dbus_listener_data_method method;
	dbus_listener_data_error error;
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
