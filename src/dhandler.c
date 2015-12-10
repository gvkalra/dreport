/*
 * Each handler is free to set "elm.swallow.content" part of layout
 * The parent of layout is a naviframe
 */

#include "dcore.h"
#include "dhandler.h"
#include "dutil.h"
#include <dlog.h>
#include <Elementary.h>

static int
_compare_cb(const void *data1, const void *data2)
{
	   return strcoll(elm_object_item_part_text_get(data1, "default"),
			   elm_object_item_part_text_get(data2, "default"));
}

static void
_object_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *content;
	GDBusConnection *conn = NULL;
	gchar **names = NULL;
	int iter = 0;

	GBusType bus_type = GPOINTER_TO_UINT(data);
	Evas_Object *list = obj;
	Evas_Object *layout = elm_object_parent_widget_get(list);
	Evas_Object *parent = elm_object_parent_widget_get(layout);
	const char *service = elm_object_item_part_text_get(event_info, "default");

	conn = dbus_setup_connection(bus_type);
	names = dbus_get_object_paths(conn, service);

	if (names == NULL) {
		content = create_nocontent(parent, "Error!");
		goto EXIT;
	}

	/* Layout content */
	content = elm_list_add(layout);
	evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(content, EVAS_HINT_FILL, EVAS_HINT_FILL);

	while (names[iter]) {
		elm_list_item_sorted_insert(content, names[iter], NULL, NULL, NULL, NULL, _compare_cb);
		iter++;
	}
	evas_object_show(content);

EXIT:
	elm_object_part_content_set(layout, "elm.swallow.content", content);

	if (names != NULL)
		g_strfreev(names);
	if (conn != NULL)
		dbus_close_connection(conn);
}

static void
_bus_menu_handler(GBusType bus_type, Evas_Object *layout)
{
	Evas_Object *content;
	GDBusConnection *conn = NULL;
	gchar **names = NULL;
	int iter = 0;
	Evas_Object *parent = elm_object_parent_widget_get(layout);

	conn = dbus_setup_connection(bus_type);
	names = dbus_get_names(conn, FALSE);

	if (names == NULL) {
		content = create_nocontent(parent, "Error!");
		goto EXIT;
	}

	/* Layout content */
	content = elm_list_add(layout);
	evas_object_size_hint_weight_set(content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(content, EVAS_HINT_FILL, EVAS_HINT_FILL);

	while (names[iter]) {
		elm_list_item_sorted_insert(content, names[iter], NULL, NULL,
				_object_cb, GUINT_TO_POINTER(bus_type), _compare_cb);
		iter++;
	}
	evas_object_show(content);

EXIT:
	elm_object_part_content_set(layout, "elm.swallow.content", content);

	if (names != NULL)
		g_strfreev(names);
	if (conn != NULL)
		dbus_close_connection(conn);
}

void
system_menu_handler(Evas_Object *layout)
{
	_bus_menu_handler(G_BUS_TYPE_SYSTEM, layout);
}

void
session_menu_handler(Evas_Object *layout)
{
	_bus_menu_handler(G_BUS_TYPE_SESSION, layout);
}

void
about_menu_handler(Evas_Object *layout)
{
	Evas_Object *nocontent;
	Evas_Object *parent = elm_object_parent_widget_get(layout);

	/* Center View */
	nocontent = create_nocontent(parent, "About Handler");
	elm_object_part_content_set(layout, "elm.swallow.content", nocontent);
}

void
feedback_menu_handler(Evas_Object *layout)
{
	Evas_Object *nocontent;
	Evas_Object *parent = elm_object_parent_widget_get(layout);

	/* Center View */
	nocontent = create_nocontent(parent, "Feedback Handler");
	elm_object_part_content_set(layout, "elm.swallow.content", nocontent);
}
