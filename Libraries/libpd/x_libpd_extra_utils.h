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

// (re)size an array by name; sizes <= 0 are clipped to 1
// returns 0 on success or negative error code if non-existent
EXTERN int libpd_array_resize(void* garray, long size);

// read n values from named src array and write into dest starting at an offset
// note: performs no bounds checking on dest
// returns 0 on success or a negative error code if the array is non-existent
// or offset + n exceeds range of array
EXTERN int libpd_array_read(float* dest, void* garray, int offset, int n);

// read n values from src and write into named dest array starting at an offset
// note: performs no bounds checking on src
// returns 0 on success or a negative error code if the array is non-existent
// or offset + n exceeds range of array
EXTERN int libpd_array_write(void* garray, int offset,
    float const* src, int n);


unsigned int libpd_iemgui_get_background_color(void* ptr);
unsigned int libpd_iemgui_get_foreground_color(void* ptr);
unsigned int libpd_iemgui_get_label_color(void* ptr);

void libpd_iemgui_set_background_color(void* ptr, char const* hex);
void libpd_iemgui_set_foreground_color(void* ptr, char const* hex);
void libpd_iemgui_set_label_color(void* ptr, char const* hex);

float libpd_get_canvas_font_height(t_canvas* cnv);

int libpd_process_nodsp(void);

unsigned int convert_from_iem_color(int const color);
unsigned int convert_to_iem_color(char const* hex);


#ifdef __cplusplus
}
#endif
