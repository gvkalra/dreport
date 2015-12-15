#include "dutil.h"
#include "dhandler.h"

#include <app.h>
#include <dlog.h>
#include <Elementary.h>
#include <efl_extension.h>
#include <glib.h>
#include <system_settings.h>

typedef struct appdata {
	/* Graphical Objects */
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *layout;
	Evas_Object *nf;
} appdata_s;

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
panel_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *panel = data;
	Evas_Object *layout = elm_object_parent_widget_get(panel);
	Elm_List_Item *it = event_info;
	const char *label = elm_object_item_part_text_get(it, "default");

	if(!g_strcmp0(label, "System Bus")) {
		system_menu_handler(layout);
	} else if(!g_strcmp0(label, "Session Bus")) {
		session_menu_handler(layout);
	} else if(!g_strcmp0(label, "Statistics")) {
		stats_menu_handler(layout);
	} else if(!g_strcmp0(label, "About")) {
		about_menu_handler(layout);
	} else if(!g_strcmp0(label, "Feedback")) {
		feedback_menu_handler(layout);
	}

	elm_panel_hidden_set(panel, EINA_TRUE);
}

static void
btn_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *panel = data;
	if (!elm_object_disabled_get(panel))
		elm_panel_toggle(panel);
}

static Evas_Object*
create_panel(Evas_Object *parent)
{
	Evas_Object *panel, *list;

	/* Panel */
	panel = elm_panel_add(parent);
	elm_panel_scrollable_set(panel, EINA_TRUE);

	/* Default is visible, hide the content in default. */
	elm_panel_hidden_set(panel, EINA_TRUE);
	evas_object_show(panel);

	/* Panel content */
	list = elm_list_add(panel);
	evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_list_item_append(list, "System Bus", NULL, NULL, panel_selected_cb, panel);
	elm_list_item_append(list, "Session Bus", NULL, NULL, panel_selected_cb, panel);
	elm_list_item_append(list, "Statistics", NULL, NULL, panel_selected_cb, panel);
	elm_list_item_append(list, "About", NULL, NULL, panel_selected_cb, panel);
	elm_list_item_append(list, "Feedback", NULL, NULL, panel_selected_cb, panel);
	evas_object_show(list);

	elm_object_content_set(panel, list);

	return panel;
}

static void
panel_scroll_cb(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Panel_Scroll_Info *ev = event_info;
	Evas_Object *bg = data;
	int col = 127 * ev->rel_x;

	/* Change color for background dim */
	evas_object_color_set(bg, 0, 0, 0, col);
}

static void
panel_unhold_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *panel = data;
	if (panel == NULL)
		return;
	elm_object_disabled_set(panel, EINA_TRUE);
}

static void
panel_hold_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	Evas_Object *panel = data;
	if (panel == NULL)
		return;
	elm_object_disabled_set(panel, EINA_FALSE);
}

static void
app_exit_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void create_drawer_view(Evas_Object *nf)
{
	Evas_Object *layout, *bg, *nocontent, *panel, *btn;
	Elm_Object_Item *nf_it;

	/* Drawer Layout */
	layout = elm_layout_add(nf);
	elm_layout_theme_set(layout, "layout", "drawer", "panel");
	evas_object_show(layout);

	/* Panel */
	panel = create_panel(layout);
	elm_panel_orient_set(panel, ELM_PANEL_ORIENT_LEFT);
	elm_object_part_content_set(layout, "elm.swallow.left", panel);

	/* Panel Background (Dimmed Area) */
	bg = evas_object_rectangle_add(evas_object_evas_get(layout));
	evas_object_color_set(bg, 0, 0, 0, 0);
	evas_object_show(bg);

	evas_object_smart_callback_add(panel, "scroll", panel_scroll_cb, bg);
	elm_object_part_content_set(layout, "elm.swallow.bg", bg);

	/* Center View */
	nocontent = create_nocontent(nf, "Select option from drawer menu");
	elm_object_part_content_set(layout, "elm.swallow.content", nocontent);

	/* Push the Layout as Naviframe item */
	nf_it = elm_naviframe_item_push(nf, "Tizen DBus Inspector", NULL, NULL, layout, "drawers");

	/* hold & unhold signal callback */
	elm_object_signal_callback_add(panel, "elm,state,hold", "elm", panel_hold_cb, NULL);
	elm_object_signal_callback_add(panel, "elm,state,unhold", "elm", panel_unhold_cb, NULL);

	/* left panel toggle button */
	btn = elm_button_add(nf);
	elm_object_style_set(btn, "naviframe/drawers");
	evas_object_smart_callback_add(btn, "clicked", btn_cb, panel);
	elm_object_item_part_content_set(nf_it, "drawers", btn);
}

static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);
	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	elm_win_conformant_set(ad->win, EINA_TRUE);

	/* Conformant */
	ad->conform = elm_conformant_add(ad->win);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Indicator */
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);

	/* Base Layout */
	ad->layout = elm_layout_add(ad->conform);
	evas_object_size_hint_weight_set(ad->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_layout_theme_set(ad->layout, "layout", "application", "default");
	evas_object_show(ad->layout);

	elm_object_content_set(ad->conform, ad->layout);

	/* Naviframe */
	ad->nf = elm_naviframe_add(ad->layout);
	create_drawer_view(ad->nf);
	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_BACK, app_exit_cb, NULL);
	elm_object_part_content_set(ad->layout, "elm.swallow.content", ad->nf);

	/* Show window after base GUI is set up */
	evas_object_show(ad->win);
}

static bool
app_create(void *data)
{
	appdata_s *ad = data;
	create_base_gui(ad);

	return true;
}

static void
app_terminate(void *data)
{
	appdata_s *ad = data;
}

static void
app_control(app_control_h app_control, void *data)
{
	appdata_s *ad = data;
	elm_win_activate(ad->win);
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	ui_app_lifecycle_callback_s event_callback = {0,};
	int ret;

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = NULL;
	event_callback.resume = NULL;
	event_callback.app_control = app_control;

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() failed. err = %d", ret);
	}

	return ret;
}
