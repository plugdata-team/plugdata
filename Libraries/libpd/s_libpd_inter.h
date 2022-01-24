#pragma once
#include <m_pd.h>

typedef void(*pd_gui_callback)(void*, void*);


void register_gui_trigger(t_pdinstance* instance, void* target, pd_gui_callback callback);

void update_gui();
