#include "uris.h"
#include "Spectrum.h"

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/ui/ui.h>
#include <lv2/urid/urid.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

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
    SignalViewURIs* uris;
    uint8_t         obj_buf[4096];

    LV2UI_Write_Function write;
    LV2UI_Controller     controller;
    const char* bundle_path;

    bool  state_valid;
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
    void setupPugl(void* parent);
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
    void port_event(
        uint32_t port_index,
        uint32_t buffer_size,
        uint32_t format,
        const void *buffer);
    
    PuglStatus onEvent(const PuglEvent* event);
    PuglNativeView getNativeView(void);
    int ui_idle(void);
};
