#include "dcore.h"
#include <dlog.h>

GDBusConnection *
dbus_setup_connection(GBusType bus_type)
{
	GDBusConnection *conn = NULL;
	GError *error = NULL;

	conn = g_bus_get_sync(bus_type, NULL, &error);
	g_assert_no_error(error);

	return conn;
}

static void
__dbus_get_object_paths_internal(GDBusConnection *connection, gchar *service,
		gchar *root, GPtrArray *array)
{
	gchar **obj_paths = NULL;
	GDBusProxy *proxy = NULL;
	const gchar *xml_data = NULL;
	GDBusNodeInfo *node_info = NULL;
	guint iter = 0;
	gchar *node_path = NULL;
	GDBusNodeInfo **nodes = NULL;
	GError *error = NULL;
	GVariant *result = NULL;

	/* Setup proxy */
	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
				service, root, service, NULL, &error);
	g_assert_no_error(error);
	g_assert(proxy);

	/* Introspect */
	result = g_dbus_proxy_call_sync(proxy, "org.freedesktop.DBus.Introspectable.Introspect",
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	g_assert_no_error(error);
	g_assert(result);
	g_variant_get(result, "(&s)", &xml_data);

	/* Parse XML */
	node_info = g_dbus_node_info_new_for_xml(xml_data, &error);
	g_assert_no_error(error);
	g_assert(node_info);

	nodes = node_info->nodes;
	if (nodes == NULL)
		goto EXIT;

	/* Add & recurse */
	while(nodes[iter]) {
		if (!g_strcmp0(root, "/"))
			node_path = g_strconcat(root, nodes[iter]->path, NULL);
		else
			node_path = g_strconcat(root, "/", nodes[iter]->path, NULL);

		g_ptr_array_add(array, node_path);
		__dbus_get_object_paths_internal(connection, service, node_path, array);
		iter++;
	}

EXIT:
	/* Cleanup */
	g_object_unref(proxy);
	g_variant_unref(result);
	g_dbus_node_info_unref(node_info);
}

gchar **
dbus_get_object_paths(GDBusConnection *connection, gchar *service)
{
	gchar **obj_paths = NULL;
	GPtrArray *array = NULL;

	array = g_ptr_array_new();

	/* Add root */
	g_ptr_array_add(array, g_strdup("/"));

	/* Find more objects recursively */
	__dbus_get_object_paths_internal(connection, service, "/", array);

	/* Terminate array */
	g_ptr_array_add(array, NULL);
	obj_paths = (gchar **)g_ptr_array_free(array, FALSE);
	return obj_paths;
}

gchar **
dbus_get_activatable_names(GDBusConnection *connection)
{
	GDBusProxy *proxy = NULL;
	gchar **activable_names;
	GVariant *result = NULL;
	GError *error = NULL;

	/* Setup proxy */
	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
			"org.freedesktop.DBus", "/", "org.freedesktop.DBus",
			NULL, &error);
	g_assert_no_error(error);
	g_assert(proxy);

	/* Get Names */
	result = g_dbus_proxy_call_sync(proxy, "org.freedesktop.DBus.ListActivatableNames",
			g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	g_assert_no_error(error);
	g_assert(result);
	g_variant_get(result, "(^as)", &activable_names);

	/* Cleanup */
	g_variant_unref(result);
	g_object_unref(proxy);

	return activable_names;
}

gchar **
dbus_get_names(GDBusConnection *connection, gboolean allow_anonymous)
{
	GDBusProxy *proxy = NULL;
	gchar **names;
	GVariant *result = NULL;
	GError *error = NULL;

	/* Setup proxy */
	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
			"org.freedesktop.DBus", "/", "org.freedesktop.DBus",
			NULL, &error);
	g_assert_no_error(error);
	g_assert(proxy);

	/* Get Names */
	result = g_dbus_proxy_call_sync(proxy, "org.freedesktop.DBus.ListNames",
			g_variant_new("()"), G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	g_assert_no_error(error);
	g_assert(result);
	g_variant_get(result, "(^as)", &names);

	/* Cleanup */
	g_variant_unref(result);
	g_object_unref(proxy);

	if (allow_anonymous)
		return names;

	/* filter anonymous */
	{
		int iter = 0;
		GPtrArray *array = NULL;
		gchar **filtered_names = NULL;

		array = g_ptr_array_new();

		while (names[iter]) {
			if (!g_str_has_prefix(names[iter], ":")) {
				g_ptr_array_add(array, g_strdup(names[iter]));
			}

			iter++;
		}
		g_ptr_array_add(array, NULL);

		filtered_names = (gchar **)g_ptr_array_free(array, FALSE);

		g_strfreev(names);
		return filtered_names;
	}
}

void
dbus_close_connection(GDBusConnection *connection)
{
	g_object_unref(connection);
}
