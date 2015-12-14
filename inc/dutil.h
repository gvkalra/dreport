#pragma once

#include <Evas.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "dreport"

#if !defined(PACKAGE)
#define PACKAGE "com.samsung.dreport"
#endif

Evas_Object *create_nocontent(Evas_Object *parent, const char *text);
