#include "dcore.h"
#include "dutil.h"
#include <dlog.h>

struct _dbus_listener_ud {
	dbus_listener_cb s_cb;
	dbus_listener_destroy_cb d_cb;
	void *user_data;
};

GDBusConnection *
dbus_setup_connection(GBusType bus_type)
{
	GDBusConnection *conn = NULL;
	GError *error = NULL;

	conn = g_bus_get_sync(bus_type, NULL, &error);
	if (error != NULL || conn == NULL)
		goto ERROR;

	return conn;

ERROR:
	if (error != NULL) {
		dlog_print(DLOG_ERROR, LOG_TAG, "g_bus_get_sync() failed [%d]: %s",
				error->code, error->message);
		g_error_free(error);
	}
	return NULL;
}

static void
__dbus_get_object_paths_internal(GDBusConnection *connection, const gchar *service,
		gchar *root, GPtrArray *array)
{
	GDBusProxy *proxy = NULL;
	const gchar *xml_data = NULL;
	GDBusNodeInfo *node_info = NULL;
	guint iter = 0;
	gchar *node_path = NULL;
	GDBusNodeInfo **nodes = NULL;
	GError *error = NULL;
	GVariant *result = NULL;

	/* Setup proxy */
	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START, NULL,
				service, root, service, NULL, &error);
	if (error != NULL || proxy == NULL)
		goto EXIT;

	/* Introspect */
	result = g_dbus_proxy_call_sync(proxy, "org.freedesktop.DBus.Introspectable.Introspect",
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if (error != NULL || result == NULL)
		goto EXIT;

	g_variant_get(result, "(&s)", &xml_data);

	/* Parse XML */
	node_info = g_dbus_node_info_new_for_xml(xml_data, &error);
	if (error != NULL || node_info == NULL)
		goto EXIT;

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
	if (node_info != NULL)
		g_dbus_node_info_unref(node_info);
	g_error_free(error);
}

gchar **
dbus_get_object_paths(GDBusConnection *connection, const gchar *service)
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

GSList *
dbus_get_stats_summary(GDBusConnection *connection)
{
	GSList *list = NULL;
	dbus_stats_data_item *data = NULL;
	GDBusProxy *proxy = NULL;
	GVariant *value = NULL;
	GVariantIter *iter = NULL;
	const gchar *key = NULL;
	GVariant *result = NULL;
	GError *error = NULL;

	/* Setup proxy */
	proxy = g_dbus_proxy_new_sync(connection, G_DBUS_PROXY_FLAGS_NONE, NULL,
			"org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
			NULL, &error);
	g_assert_no_error(error);
	g_assert(proxy);

	/* Get Stats */
	result = g_dbus_proxy_call_sync(proxy, "org.freedesktop.DBus.Debug.Stats.GetStats",
			NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	g_assert_no_error(error);
	g_assert(result);
	g_variant_get(result, "(a{sv})", &iter);

	while (g_variant_iter_loop(iter, "{sv}", &key, &value)) {
		data = g_new0(dbus_stats_data_item, 1);

		data->key = g_strdup(key);
		data->value = g_variant_get_uint32(value);
		dlog_print(DLOG_VERBOSE, LOG_TAG, "key = %s, value = %u",
				data->key, data->value);

		list = g_slist_prepend(list, data);
	}

	/* Cleanup */
	g_variant_iter_free(iter);
	g_variant_unref(result);
	g_object_unref(proxy);

	return list;
}

static void
_dbus_listener_cb_internal(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path,
		const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data)
{
	struct _dbus_listener_ud *ud = user_data;
	dbus_listener_data_item *data = g_new0(dbus_listener_data_item, 1);

	data->sender_name = sender_name;
	data->object_path = object_path;
	data->interface_name = interface_name;
	data->signal_name = signal_name;
	data->message = g_strdup("MESSAGE");

	/* Invoke registered callback */
	ud->s_cb(connection, data, ud->user_data);

	/* Cleanup */
	g_free(data->message);
	g_free(data);
}

static void
_dbus_listener_destroy_cb_internal(gpointer user_data)
{
	struct _dbus_listener_ud *ud = user_data;

	/* Invoke registered callback and then free self */
	ud->d_cb(ud->user_data);
	g_free(ud);
}

gint
dbus_register_listener(GDBusConnection *connection, dbus_listener_cb s_cb, void *user_data, dbus_listener_destroy_cb d_cb)
{
	guint id;

	struct _dbus_listener_ud *ud = g_new0(struct _dbus_listener_ud, 1);
	ud->s_cb = s_cb;
	ud->d_cb = d_cb;
	ud->user_data = user_data;

	id = g_dbus_connection_signal_subscribe(connection, NULL, NULL, NULL, NULL,
			NULL, G_DBUS_SIGNAL_FLAGS_NONE,
			_dbus_listener_cb_internal, ud, _dbus_listener_destroy_cb_internal);

	return id == 0 ? (-1) : (id);
}

void
dbus_deregister_listener(GDBusConnection *connection, gint id)
{
	if (id <= 0)
		return;

	g_dbus_connection_signal_unsubscribe(connection, (guint)id);
}

void
dbus_close_connection(GDBusConnection *connection)
{
	g_object_unref(connection);
}
