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
#include <memory>

class SignalView
{
    // Port buffers
    float *input[2];
    float *output[2];
    const LV2_Atom_Sequence* control;
    LV2_Atom_Sequence*       notify;

    // Atom forge and URI mapping
    LV2_URID_Map*                   map;
    std::unique_ptr<SignalViewURIs> uris;
    LV2_Atom_Forge                  forge;
    LV2_Atom_Forge_Frame            seq_frame;

    float vec_buffer[32000];

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
        const size_t  n_samples,
        const float*  data0,
        const float*  data1);
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
