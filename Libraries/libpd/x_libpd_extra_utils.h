/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#ifdef __cplusplus


extern "C"
{
#endif
#include <m_pd.h>
#include <z_libpd.h>



void* libpd_create_canvas(const char* name, const char* path);

char const* libpd_get_object_class_name(void* ptr);
void libpd_get_object_text(void* ptr, char** text, int* size);
void libpd_get_object_bounds(void* patch, void* ptr, int* x, int* y, int* w, int* h);

t_garray* libpd_array_get_byname(char const* name);
char const* libpd_array_get_name(void* ptr);
char const* libpd_array_get_unexpanded_name(void* ptr);

void libpd_array_get_scale(char const* name, float* min, float* max);
void libpd_array_set_scale(char const* name, float min, float max);

int libpd_array_get_size(char const* name);
int libpd_array_get_style(char const* name);

unsigned int libpd_iemgui_get_background_color(void* ptr);
unsigned int libpd_iemgui_get_foreground_color(void* ptr);
unsigned int libpd_iemgui_get_label_color(void* ptr);

void libpd_iemgui_set_background_color(void* ptr, const char* hex);
void libpd_iemgui_set_foreground_color(void* ptr, const char* hex);
void libpd_iemgui_set_label_color(void* ptr, const char* hex);

float libpd_get_canvas_font_height(t_canvas* cnv);


#ifdef __cplusplus
}
#endif
