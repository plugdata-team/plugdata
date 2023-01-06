#pragma once
#include <m_pd.h>

typedef void (*pd_gui_callback)(void*, void*);
typedef void (*pd_panel_callback)(void*, int, char const*, char const*);
typedef void (*pd_openfile_callback)(void*, char const*);
typedef void (*pd_message_callback)(void*, void*, t_symbol*, int, t_atom*);


void trigger_open_file(const char* file);

void register_gui_triggers(t_pdinstance* instance, void* target, pd_gui_callback gui_callback, pd_panel_callback panel_callback, pd_openfile_callback openfile_callback, pd_message_callback message_callback);

void create_panel(int openpanel, char const* path, char const* snd);
