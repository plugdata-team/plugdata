#pragma once
#include <m_pd.h>


typedef void(*pd_gui_callback)(void*, void*);
typedef void(*pd_panel_callback)(void*, int, const char*, const char*);

void register_gui_triggers(t_pdinstance* instance, void* target, pd_gui_callback gui_callback, pd_panel_callback panel_callback);

void update_gui();
