/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#ifdef __cplusplus

extern "C" {
#endif
#include <m_pd.h>
#include "z_libpd.h"

void* libpd_create_canvas(char const* name, char const* path);

char const* libpd_get_object_class_name(void* ptr);
void libpd_get_object_text(void* ptr, char** text, int* size);
void libpd_get_object_bounds(void* patch, void* ptr, int* x, int* y, int* w, int* h);

t_garray* libpd_array_get_byname(char const* name);
char const* libpd_array_get_name(void* array);
char const* libpd_array_get_unexpanded_name(void* array);

void libpd_array_get_scale(void* array, float* min, float* max);
void libpd_array_set_scale(void* array, float min, float max);

int libpd_array_get_size(void* array);
int libpd_array_get_style(void* array);
int libpd_array_get_saveit(void* array);

unsigned int libpd_iemgui_get_background_color(void* ptr);
unsigned int libpd_iemgui_get_foreground_color(void* ptr);
unsigned int libpd_iemgui_get_label_color(void* ptr);

void libpd_iemgui_set_background_color(void* ptr, char const* hex);
void libpd_iemgui_set_foreground_color(void* ptr, char const* hex);
void libpd_iemgui_set_label_color(void* ptr, char const* hex);

float libpd_get_canvas_font_height(t_canvas* cnv);

#ifdef __cplusplus
}
#endif
