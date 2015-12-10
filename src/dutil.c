#include "dutil.h"

#include <Elementary.h>

Evas_Object*
create_nocontent(Evas_Object *parent, const char *text)
{
	Evas_Object *layout, *scroller;

	/* Scroller */
	scroller = elm_scroller_add(parent);
	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	/* NoContent Layout */
	layout = elm_layout_add(scroller);
	elm_layout_theme_set(layout, "layout", "nocontents", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_object_part_text_set(layout, "elm.text", text);

	elm_object_content_set(scroller, layout);

	return scroller;
}
