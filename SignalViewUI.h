/*
    SignalView LV2 analysis plugin
    Copyright (C) 2025  Timothy William Krause
    mailto:tmkrs4482@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "uris.h"
#include "Spectrum.h"
#include "Semaphore.h"

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <thread>
#include <functional>
#include <semaphore>

#define GLAD_GL_IMPLEMENTATION
#include "glad/gl.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#define BUTTON_LOG 1
#define BUTTON_MOTION 0

PuglStatus onEvent(PuglView* view, const PuglEvent* event);

class SignalViewUI
{
    LV2_Atom_Forge  forge;
    LV2_URID_Map*   map;
    LV2_Log_Logger  logger;
    LV2_Log_Log*    logger_log;
    SignalViewURIs* uris;
    uint8_t         obj_buf[4096];

    LV2UI_Write_Function write;
    LV2UI_Controller     controller;
    const char* bundle_path;
    void* parentXWindow;
    std::thread ui_thread;
    Semaphore  state_sem; // Spectrum allocation must wait for state
    Semaphore  view_sem; // instantiate needs to wait for the view

    bool  state_valid;
    bool  view_ready;
    bool  quit;
    float rate;
    float dB_min;
    float dB_max;
    float linFreq;
    bool  log;

    PuglWorld* world;
    PuglView*  view;
    int        width;
    int        height;
    int        h1;
    int        h2;
    bool       mousing;
    int        x_last;
    int        y_last;

    std::unique_ptr<Spectrum> spectrum;
    void setSpectrum(void);

    public:
    SignalViewUI(
        const LV2UI_Descriptor *descriptor,
        const char *plugin_uri,
        const char *bundle_path,
        LV2UI_Write_Function write_function,
        LV2UI_Controller controller,
        LV2UI_Widget *widget,
        const LV2_Feature *const *features);
    ~SignalViewUI();

    private:
    void setupGL(void);
    void teardownGL(void);
    void onConfigure(int width, int height);
    void onExpose(void);
    void onScroll(int y, int dy);
    void onButtonPress(const PuglButtonEvent* e);
    void onButtonRelease(const PuglButtonEvent* e);
    void onMotion(const PuglMotionEvent* e);

    void send_ui_state(void);
    void send_ui_disable(void);
    void send_ui_enable(void);
    void send_ui_send_state(void);
    void recv_raw_audio(const LV2_Atom_Object* obj);
    void recv_ui_state(const LV2_Atom_Object* obj);

    public:
    void setupPugl(void);
    void port_event(
        uint32_t port_index,
        uint32_t buffer_size,
        uint32_t format,
        const void *buffer);
    
    PuglStatus onEvent(PuglView* view, const PuglEvent* event);
    PuglNativeView getNativeView(void);
    int ui_idle(void);
    void eventLoop(void);
};

void ui_thread_func(SignalViewUI* ui);



