#ifndef URIS_H
#define URIS_H

#include <lv2/atom/atom.h>
#include <lv2/parameters/parameters.h>
#include <lv2/urid/urid.h>

#define SIGNAL_VIEW_URI "https://twkrause.ca/plugins/SignalView"
#define SIGNAL_VIEW_UI_URI SIGNAL_VIEW_URI "#ui"

struct SignalViewURIs
{
    // URIs defined in LV2 specifications
    LV2_URID atom_Vector;
    LV2_URID atom_Bool;
    LV2_URID atom_Float;
    LV2_URID atom_Int;
    LV2_URID atom_eventTransfer;
    LV2_URID param_sampleRate;

    /* URIs defined for this plugin.  It is best to reuse existing URIs as
     much as possible, but plugins may need more vocabulary specific to their
     needs.  These are used as types and properties for plugin:UI
     communication, as well as for saving state. */
    LV2_URID RawAudio;
    LV2_URID nChannels;
    LV2_URID audioData;
    LV2_URID ui_On;
    LV2_URID ui_Off;
    LV2_URID ui_SendState;
    LV2_URID ui_State;
    LV2_URID ui_dB_min;
    LV2_URID ui_dB_max;
    LV2_URID ui_log;
    LV2_URID ui_linFreq;

    SignalViewURIs(LV2_URID_Map* map)
    {
        atom_Vector        = map->map(map->handle, LV2_ATOM__Vector);
        atom_Bool          = map->map(map->handle, LV2_ATOM__Bool);
        atom_Float         = map->map(map->handle, LV2_ATOM__Float);
        atom_Int           = map->map(map->handle, LV2_ATOM__Int);
        atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
        param_sampleRate   = map->map(map->handle, LV2_PARAMETERS__sampleRate);

        /* Note the convention that URIs for types are capitalized, and URIs for
           everything else (mainly properties) are not, just as in LV2
           specifications. */
        RawAudio     = map->map(map->handle, SIGNAL_VIEW_URI "#RawAudio");
        audioData    = map->map(map->handle, SIGNAL_VIEW_URI "#audioData");
        nChannels    = map->map(map->handle, SIGNAL_VIEW_URI "#nChannels");
        ui_On        = map->map(map->handle, SIGNAL_VIEW_URI "#UIOn");
        ui_Off       = map->map(map->handle, SIGNAL_VIEW_URI "#UIOff");
        ui_SendState = map->map(map->handle, SIGNAL_VIEW_URI "#UISendState");
        ui_State     = map->map(map->handle, SIGNAL_VIEW_URI "#UIState");
        ui_dB_min    = map->map(map->handle, SIGNAL_VIEW_URI "#ui-dB-min");
        ui_dB_max    = map->map(map->handle, SIGNAL_VIEW_URI "#ui-dB-max");
        ui_log       = map->map(map->handle, SIGNAL_VIEW_URI "#ui-log");
        ui_linFreq   = map->map(map->handle, SIGNAL_VIEW_URI "#ui-linFreq");
    }

};

#endif
