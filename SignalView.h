#include "uris.h"

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/state/state.h>
#include <lv2/urid/urid.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class SignalView
{
    // Port buffers
    float *input[2];
    float *output[2];
    const LV2_Atom_Sequence* control;
    LV2_Atom_Sequence*       notify;

    // Atom forge and URI mapping
    LV2_URID_Map*        map;
    SignalViewURIs*      uris;
    LV2_Atom_Forge       forge;
    LV2_Atom_Forge_Frame seq_frame;

    double rate;

    // UI state
    bool ui_active;
    bool send_settings_to_ui;
    float dB_min;
    float dB_max;
    bool  log;
    float linFreq;

public:
    SignalView(
        const LV2_Descriptor*     descriptor,
        double                    rate,
        const char*               bundle_path,
        const LV2_Feature* const* features);

    void connect_port(uint32_t port, void *data);
    void tx_rawaudio(
        const int32_t channel,
        const size_t  n_samples,
        const float*  data);
    void run(uint32_t n_samples);
    LV2_State_Status state_save(
        LV2_State_Store_Function  store,
        LV2_State_Handle          handle,
        uint32_t                  flags,
        const LV2_Feature* const* features);
    LV2_State_Status state_restore(
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature* const*   features);
};

enum PortIndex {
    PORT_CONTROL = 0,
    PORT_NOTIFY  = 1,
    PORT_INPUT0  = 2,
    PORT_INPUT1  = 3,
    PORT_OUTPUT0 = 4,
    PORT_OUTPUT1 = 5,
    PORT_NPORTS  = 6
};
