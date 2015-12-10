#include "dreport.h"
#include "dcore.h"

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *naviframe;
	Evas_Object *system_list;
	Evas_Object *session_list;
	Evas_Object *object_paths;
	GDBusConnection *system_connection;
	GDBusConnection *session_connection;
	Evas_Object *tabbar;
	Elm_Object_Item *system_tab;
	Elm_Object_Item *session_tab;
	Elm_Object_Item *nf_it;
	gchar **system_names;
	gchar **session_names;
	gchar **object_path_names;
} appdata_s;

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static void _tab_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;

	/* clear object list TODO: cleanup code needs revisit */
	elm_object_part_content_unset(ad->naviframe, "elm.swallow.content");

	if (evas_object_visible_get(ad->object_paths))
		evas_object_hide(ad->object_paths);

	/* previous list exist */
	if (ad->object_paths) {
		elm_genlist_clear(ad->object_paths);
		ad->object_paths = NULL;
	}

	if (ad->object_path_names) {
		g_strfreev(ad->object_path_names);
		ad->object_path_names = NULL;
	}

	if (event_info == ad->system_tab){
		evas_object_hide(ad->session_list);
		elm_object_part_content_set(ad->naviframe, "elm.swallow.content", ad->system_list);
		evas_object_show(ad->system_list);
	} else {
		evas_object_hide(ad->system_list);
		elm_object_part_content_set(ad->naviframe, "elm.swallow.content", ad->session_list);
		evas_object_show(ad->session_list);
	}
}

static char *
_genlist_text_get(void *data, Evas_Object *obj, const char *part)
{
	// Check this is text for the part we're expecting
	if (strcmp(part, "elm.text") == 0)
	{
	  return strdup(data);
	}
	else {
	  return NULL;
	}
}

static void
_genlist_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Genlist_Item_Class *itc = NULL;
	appdata_s *ad = data;
	Elm_Object_Item *it = (Elm_Object_Item*) event_info;
	gchar *service = NULL;
	guint iter;
	Evas_Object *list = ((obj == ad->system_list) ? ad->system_list : ad->session_list);

	/* previous list exist */
	if (ad->object_paths) {
		elm_genlist_clear(ad->object_paths);
		ad->object_paths = NULL;
	}

	if (ad->object_path_names) {
		g_strfreev(ad->object_path_names);
		ad->object_path_names = NULL;
	}

	service = elm_object_item_data_get(it);
	ad->object_path_names = dbus_get_object_paths(ad->system_connection, service);

	/* hide current list */
	elm_object_part_content_unset(ad->naviframe, "elm.swallow.content");
	evas_object_hide(list);

	/* create list for objects */
	ad->object_paths = elm_genlist_add(ad->naviframe);
	evas_object_size_hint_weight_set(ad->object_paths, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->object_paths, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_scroller_policy_set(ad->object_paths, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	itc = elm_genlist_item_class_new();
	itc->item_style = "default_style";
	itc->func.text_get = _genlist_text_get;
	itc->func.content_get = NULL;
	itc->func.state_get = NULL;
	itc->func.del = NULL;

	for (iter = 0; ad->object_path_names != NULL && ad->object_path_names[iter] != NULL; iter++)
		elm_genlist_item_append(ad->object_paths, itc, ad->object_path_names[iter], NULL, ELM_GENLIST_ITEM_NONE, NULL, ad);

	elm_genlist_item_class_free(itc);

	elm_object_part_content_set(ad->naviframe, "elm.swallow.content", ad->object_paths);
	evas_object_show(ad->object_paths);

	elm_genlist_item_selected_set(it, EINA_FALSE);
}

static int
_compare_cb(const void *data1, const void *data2)
{
	   Elm_Object_Item *it = (Elm_Object_Item *)data1;
	   Elm_Object_Item *it2 = (Elm_Object_Item *)data2;
	   return strcoll(elm_object_item_data_get(it), elm_object_item_data_get(it2));
}

static void
create_base_gui(appdata_s *ad)
{
	int i;
	Elm_Genlist_Item_Class *itc = NULL;

	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	// naviframe
	ad->naviframe = elm_naviframe_add(ad->conform);
	evas_object_size_hint_weight_set(ad->naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->conform, ad->naviframe);
	elm_object_content_set(ad->conform, ad->naviframe);
	evas_object_show(ad->naviframe);
	ad->nf_it = elm_naviframe_item_push(ad->naviframe,"UiLayout",NULL, NULL, NULL, "tabbar/icon/notitle");

	// Tabbar
	ad->tabbar = elm_toolbar_add(ad->naviframe);
	elm_object_style_set(ad->tabbar, "tabbar");
	elm_toolbar_shrink_mode_set(ad->tabbar, ELM_TOOLBAR_SHRINK_EXPAND);
	elm_toolbar_transverse_expanded_set(ad->tabbar, EINA_TRUE);

	ad->system_tab = elm_toolbar_item_append(ad->tabbar, NULL, "System", _tab_selected_cb, ad);
	ad->session_tab = elm_toolbar_item_append(ad->tabbar, NULL, "Session", _tab_selected_cb, ad);
	elm_object_item_part_content_set(ad->nf_it, "tabbar", ad->tabbar);

	// create system list/itc
	ad->system_list = elm_genlist_add(ad->naviframe);
	evas_object_size_hint_weight_set(ad->system_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->system_list, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_scroller_policy_set(ad->system_list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
	//elm_scroller_page_show(ad->system_list, 0, 0);

	// create session list/itc
	ad->session_list = elm_genlist_add(ad->naviframe);
	evas_object_size_hint_weight_set(ad->session_list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(ad->session_list, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_scroller_policy_set(ad->session_list, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	// create item class
	itc = elm_genlist_item_class_new();
	itc->item_style = "default_style";
	itc->func.text_get = _genlist_text_get;
	itc->func.content_get = NULL;
	itc->func.state_get = NULL;
	itc->func.del = NULL;

	// add items to system list
	for (i = 0; ad->system_names != NULL && ad->system_names[i] != NULL; i++) {
			elm_genlist_item_sorted_insert(ad->system_list, itc, ad->system_names[i], NULL, ELM_GENLIST_ITEM_NONE, _compare_cb, _genlist_selected_cb, ad);
		}

	// add items to session list
	for (i = 0; ad->session_names != NULL && ad->session_names[i] != NULL; i++) {
			elm_genlist_item_sorted_insert(ad->session_list, itc, ad->session_names[i], NULL, ELM_GENLIST_ITEM_NONE, _compare_cb, _genlist_selected_cb, ad);
	}

	elm_genlist_item_class_free(itc);

	// Set the first view
	elm_toolbar_item_selected_set(ad->system_tab, EINA_TRUE);
	evas_object_show(ad->system_list);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	// set up dbus proxies
	ad->system_connection = dbus_setup_connection(G_BUS_TYPE_SYSTEM);
	ad->system_names = dbus_get_names(ad->system_connection, FALSE);


	ad->session_connection = dbus_setup_connection(G_BUS_TYPE_SESSION);
	ad->session_names = dbus_get_names(ad->session_connection, FALSE);

	create_base_gui(ad);

	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
	appdata_s *ad = data;

	dbus_close_connection(ad->system_connection);
	g_strfreev(ad->system_names);

	dbus_close_connection(ad->session_connection);
	g_strfreev(ad->session_names);

	if (ad->object_path_names) {
		g_strfreev(ad->object_path_names);
		ad->object_path_names = NULL;
	}
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
