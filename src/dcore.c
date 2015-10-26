#include "dcore.h"
#include <dlog.h>

GDBusProxy *
dbus_setup_proxy(GBusType bus_type)
{
	GDBusProxy *proxy = NULL;
	GError *error = NULL;

	// gdbus introspect --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus
	proxy = g_dbus_proxy_new_for_bus_sync(bus_type, G_DBUS_PROXY_FLAGS_NONE,
	NULL, "org.freedesktop.DBus", "/org/freedesktop/DBus",
			"org.freedesktop.DBus", NULL, &error);
	g_assert_no_error(error);
	g_assert(proxy);

	return proxy;
}

gchar **
dbus_get_activatable_names(GDBusProxy *proxy)
{
	gchar **activable_names;
	GVariant *result = NULL;
	GError *error = NULL;

	// gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus --method org.freedesktop.DBus.ListActivatableNames
	result = g_dbus_proxy_call_sync(proxy,
			"org.freedesktop.DBus.ListActivatableNames", g_variant_new("()"),
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	g_assert_no_error(error);
	g_assert(result);
	g_variant_get(result, "(^as)", &activable_names);
	g_variant_unref(result);
	return activable_names;
}

gchar **
dbus_get_names(GDBusProxy *proxy)
{
	gchar ** names;
	GVariant *result = NULL;
	GError *error = NULL;

	// gdbus call --system --dest org.freedesktop.DBus --object-path /org/freedesktop/DBus --method org.freedesktop.DBus.ListNames
	result = g_dbus_proxy_call_sync(proxy, "org.freedesktop.DBus.ListNames",
			g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	g_assert_no_error(error);
	g_assert(result);
	g_variant_get(result, "(^as)", &names);
	g_variant_unref(result);
	return names;

}

void
dbus_close_proxy(GDBusProxy *proxy)
{
	g_object_unref(proxy);
}
