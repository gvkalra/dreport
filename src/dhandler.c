/*
 * Each handler is free to set "elm.swallow.content" part of layout
 * The parent of layout is a naviframe
 */

#include "dcore.h"
#include "dhandler.h"
#include "dutil.h"
#include <dlog.h>
#include <Elementary.h>
#include <efl_extension.h>
#include <app.h>

static int
_compare_cb(const void *data1, const void *data2)
{
	return strcoll(elm_object_item_part_text_get(data1, "default"),
			elm_object_item_part_text_get(data2, "default"));
}

static void
_back_key_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *content_nf = obj;
	elm_naviframe_item_pop(content_nf);
}

static Eina_Bool
_nf_item_pop_cb(void *data, Elm_Object_Item *it)
{
	ui_app_exit();
	return EINA_FALSE;
}

/* TODO: explore "navigationbar" style of toolbar */
static const char *
_get_bus_header(GBusType bus_type)
{
	if (bus_type == G_BUS_TYPE_SESSION)
		return "Session Bus";
	else if (bus_type == G_BUS_TYPE_SYSTEM)
		return "System Bus";

	g_assert_not_reached();
}

/* TODO: elm_object_parent_widget_get() on service_list created by elm_list_add()
 * with content_nf as parent in _bus_menu_handler() is not returning content_nf as the parent.
 * Am clueless!
 */
static Evas_Object *nf_wtf;

static void
_object_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *obj_list;
	GDBusConnection *conn = NULL;
	gchar **names = NULL;
	int iter = 0;

	GBusType bus_type = GPOINTER_TO_UINT(data);
	Evas_Object *service_list = obj;
	Evas_Object *content_nf = nf_wtf;
	Evas_Object *layout = elm_object_parent_widget_get(content_nf);
	Evas_Object *parent = elm_object_parent_widget_get(layout);
	const char *service = elm_object_item_part_text_get(event_info, "default");

	dlog_print(DLOG_ERROR, LOG_TAG, "content_nf = %p, service_list = %p",
				content_nf, service_list);

	conn = dbus_setup_connection(bus_type);
	names = dbus_get_object_paths(conn, service);

	if (names == NULL) {
		obj_list = create_nocontent(parent, "Error!");
		goto EXIT;
	}

	/* Layout content */
	obj_list = elm_list_add(content_nf);

	while (names[iter]) {
		elm_list_item_sorted_insert(obj_list, names[iter], NULL, NULL,
				NULL, NULL, _compare_cb);
		iter++;
	}

EXIT:
	elm_naviframe_item_push(content_nf, service, NULL, NULL, obj_list, NULL);

	if (names != NULL)
		g_strfreev(names);
	if (conn != NULL)
		dbus_close_connection(conn);
}

static void
_bus_menu_handler(GBusType bus_type, Evas_Object *layout)
{
	Evas_Object *content_nf;
	Evas_Object *service_list;
	GDBusConnection *conn = NULL;
	gchar **names = NULL;
	int iter = 0;
	Evas_Object *parent = elm_object_parent_widget_get(layout);
	Elm_Object_Item *nf_it;

	conn = dbus_setup_connection(bus_type);
	names = dbus_get_names(conn, FALSE);

	if (names == NULL) {
		content_nf = create_nocontent(parent, "Error!");
		goto EXIT;
	}

	/* Content */
	content_nf = elm_naviframe_add(layout);
	elm_naviframe_prev_btn_auto_pushed_set(content_nf, EINA_TRUE);
	eext_object_event_callback_add(content_nf, EEXT_CALLBACK_BACK, _back_key_cb, NULL);
	service_list = elm_list_add(content_nf);

	nf_wtf = content_nf;

	dlog_print(DLOG_ERROR, LOG_TAG, "layout = %p, content_nf = %p, service_list = %p",
			layout, content_nf, service_list);

	while (names[iter]) {
		elm_list_item_sorted_insert(service_list, names[iter], NULL, NULL,
				_object_cb, GUINT_TO_POINTER(bus_type), _compare_cb);
		iter++;
	}
	nf_it = elm_naviframe_item_push(content_nf, _get_bus_header(bus_type), NULL, NULL, service_list, NULL);
	elm_naviframe_item_pop_cb_set(nf_it, _nf_item_pop_cb, NULL);

EXIT:
	elm_object_part_content_set(layout, "elm.swallow.content", content_nf);

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
listen_menu_handler(Evas_Object *layout)
{
	Evas_Object *nocontent;
	Evas_Object *parent = elm_object_parent_widget_get(layout);

	/* Center View */
	nocontent = create_nocontent(parent, "Listen Mode Handler");
	elm_object_part_content_set(layout, "elm.swallow.content", nocontent);
}

static char *
_stats_data_label_get(void *data, Evas_Object *obj, const char *part)
{
	dbus_stats_data_item *item = data;

	if (!g_strcmp0(part, "elm.text")) {
		return strdup(item->key);
	} else if (!g_strcmp0(part, "elm.text.end")) {
		char *value = NULL;
		gchar *_value = g_strdup_printf("%u", item->value);

		/* should not use g_strdup() */
		value = strdup(_value);
		g_free(_value);

		return value;
	} else {
		return NULL;
	}
}

static char *
_stats_header_label_get(void *data, Evas_Object *obj, const char *part)
{
	GBusType bus_type = GPOINTER_TO_UINT(data);

	if (!g_strcmp0(part, "elm.text")) {
		return bus_type == G_BUS_TYPE_SYSTEM ?
				strdup("System Bus") : strdup("Session Bus");
	} else {
		return NULL;
	}
}

static void
_stats_data_item_del(void *data, Evas_Object *obj)
{
	dbus_stats_data_item *item = data;
	g_free(item->key);
	g_free(item);
}

void
stats_menu_handler(Evas_Object *layout)
{
	GDBusConnection *conn = NULL;
	GSList *stats = NULL;
	GSList *iter = NULL;
	Evas_Object *list = NULL;
	Elm_Genlist_Item_Class *itc_stats, *itc_header = NULL;

	/* List */
	list = elm_genlist_add(layout);
	elm_object_style_set(list, "default");

	/* Header Style */
	itc_header = elm_genlist_item_class_new();
	itc_header->item_style = "default";
	itc_header->func.text_get = _stats_header_label_get;
	itc_header->func.content_get = NULL;
	itc_header->func.del = NULL;

	/* Data Style */
	itc_stats = elm_genlist_item_class_new();
	itc_stats->item_style = "group_index";
	itc_stats->func.text_get = _stats_data_label_get;
	itc_stats->func.content_get = NULL;
	itc_stats->func.del = _stats_data_item_del;

	/* System */
	conn = dbus_setup_connection(G_BUS_TYPE_SYSTEM);
	stats = dbus_get_stats_summary(conn);
	elm_genlist_item_append(list, itc_header, GUINT_TO_POINTER(G_BUS_TYPE_SYSTEM),
					NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	for (iter = g_slist_nth(stats, 0); iter != NULL; iter = g_slist_next(iter)) {
		elm_genlist_item_append(list, itc_stats, iter->data,
				NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	}
	g_slist_free(stats); /* we stole all data pointers */
	dbus_close_connection(conn);

	/* Session */
	conn = dbus_setup_connection(G_BUS_TYPE_SESSION);
	stats = dbus_get_stats_summary(conn);
	elm_genlist_item_append(list, itc_header, GUINT_TO_POINTER(G_BUS_TYPE_SESSION),
					NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	for (iter = g_slist_nth(stats, 0); iter != NULL; iter = g_slist_next(iter)) {
		elm_genlist_item_append(list, itc_stats, iter->data,
				NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
	}
	g_slist_free(stats);
	dbus_close_connection(conn);

	/* Cleanup */
	elm_genlist_item_class_free(itc_stats);
	elm_genlist_item_class_free(itc_header);
	elm_object_part_content_set(layout, "elm.swallow.content", list);
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
