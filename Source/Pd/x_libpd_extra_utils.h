/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#ifndef __X_LIBPD_EXTRA_UTILS_H__
#define __X_LIBPD_EXTRA_UTILS_H__

#ifdef __cplusplus


extern "C"
{
#endif

#include <z_libpd.h>
#include <m_pd.h>


void* libpd_create_canvas(const char* name, const char* path);

char const* libpd_get_object_class_name(void* ptr);
void libpd_get_object_text(void* ptr, char** text, int* size);
void libpd_get_object_bounds(void* patch, void* ptr, int* x, int* y, int* w, int* h, int is_graph);


char const* libpd_array_get_name(void* ptr);
void libpd_array_get_scale(char const* name, float* min, float* max);
int libpd_array_get_style(char const* name);

unsigned int libpd_iemgui_get_background_color(void* ptr);
unsigned int libpd_iemgui_get_foreground_color(void* ptr);

float libpd_get_canvas_font_height(t_canvas* cnv);


#ifdef __cplusplus
}
#endif

#endif
