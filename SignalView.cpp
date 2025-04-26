#include "SignalView.h"

SignalView::SignalView(
    const LV2_Descriptor*     descriptor,
    double                    rate,
    const char*               bundle_path,
    const LV2_Feature* const* features)
{
    const char* missing = lv2_features_query(
        features,
        LV2_URID__map, &map,        true,
        NULL);

    if (missing) {
        printf("Missing feature <%s>\n", missing);
        throw;
    }

    ui_active = false;
    send_settings_to_ui = false;
    SignalView::rate = rate;

    uris = new SignalViewURIs(map);
    lv2_atom_forge_init(&forge, map);
}

void SignalView::connect_port(uint32_t port, void *data)
{
    switch((PortIndex)port){
    case PORT_CONTROL:
        control = (const LV2_Atom_Sequence*)data;
        break;
    case PORT_NOTIFY:
        notify = (LV2_Atom_Sequence*)data;
        break;
    case PORT_INPUT0:
        input[0] = (float*)data;
        break;
    case PORT_INPUT1:
        input[1] = (float*)data;
        break;
    case PORT_OUTPUT0:
        output[0] = (float*)data;
        break;
    case PORT_OUTPUT1:
        output[1] = (float*)data;
        break;
    case PORT_NPORTS:
        break;
    }
}

void SignalView::tx_rawaudio(
    const int32_t channel,
    const size_t  n_samples,
    const float*  data)
{
    LV2_Atom_Forge_Frame frame;

    // Forge container object of type 'RawAudio'
    lv2_atom_forge_frame_time(&forge, 0);
    lv2_atom_forge_object(&forge, &frame, 0, uris->RawAudio);

    // Add integer 'channelID' property
    lv2_atom_forge_key(&forge, uris->channelID);
    lv2_atom_forge_int(&forge, channel);

    // Add vector of floats 'audioData' property
    lv2_atom_forge_key(&forge, uris->audioData);
    lv2_atom_forge_vector(
        &forge, sizeof(float), uris->atom_Float, n_samples, data);

    // Close off object
    lv2_atom_forge_pop(&forge, &frame);

}

void SignalView::run(uint32_t n_samples)
{
    const uint32_t space = notify->atom.size;

    // Prepare forge buffer and initialize atom-sequence
    lv2_atom_forge_set_buffer(&forge, (uint8_t*)notify, space);
    lv2_atom_forge_sequence_head(&forge, &seq_frame, 0);

    /* Send settings to UI

     The plugin can continue to run while the UI is closed and re-opened.
     The state and settings of the UI are kept here and transmitted to the UI
     every time it asks for them or if the user initializes a 'load preset'.
  */
    if (send_settings_to_ui) {
        send_settings_to_ui = false;
        // Forge container object of type 'ui_state'
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time(&forge, 0);
        lv2_atom_forge_object(&forge, &frame, 0, uris->ui_State);

        // Add UI state as properties
        lv2_atom_forge_key(&forge, uris->ui_dB_min);
        lv2_atom_forge_float(&forge, dB_min);
        lv2_atom_forge_key(&forge, uris->ui_dB_max);
        lv2_atom_forge_float(&forge, dB_max);
        lv2_atom_forge_key(&forge, uris->ui_linFreq);
        lv2_atom_forge_float(&forge, linFreq);
        lv2_atom_forge_key(&forge, uris->ui_log);
        lv2_atom_forge_bool(&forge, (int32_t)log);
        lv2_atom_forge_key(&forge, uris->param_sampleRate);
        lv2_atom_forge_float(&forge, (float)rate);
        lv2_atom_forge_pop(&forge, &frame);
    }

    // Process incoming events from GUI
    if (control) {
        const LV2_Atom_Event* ev = lv2_atom_sequence_begin(&control->body);
        // For each incoming message...
        while (!lv2_atom_sequence_is_end(
            &control->body, control->atom.size, ev)) {
            // If the event is an atom:Blank object
            if (lv2_atom_forge_is_object_type(&forge, ev->body.type)) {
                const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
                if (obj->body.otype == uris->ui_On) {
                    // If the object is a ui-on, the UI was activated
                    ui_active           = true;
                } else if (obj->body.otype == uris->ui_SendState) {
                    send_settings_to_ui = true;
                } else if (obj->body.otype == uris->ui_Off) {
                    // If the object is a ui-off, the UI was closed
                    ui_active = false;
                } else if (obj->body.otype == uris->ui_State) {
                    // If the object is a ui-state, it's the current UI settings
                    const LV2_Atom* dB_min_atom = NULL;
                    const LV2_Atom* dB_max_atom = NULL;
                    const LV2_Atom* linFreq_atom = NULL;
                    const LV2_Atom* log_atom = NULL;
                    lv2_atom_object_get(
                        obj,
                        uris->ui_dB_min, &dB_min_atom,
                        uris->ui_dB_max, &dB_max_atom,
                        uris->ui_linFreq, &linFreq_atom,
                        uris->ui_log, &log_atom,
                        0);
                    if(dB_min_atom) {
                        dB_min = ((const LV2_Atom_Float*)dB_min_atom)->body;
                    }
                    if(dB_max_atom) {
                        dB_max = ((const LV2_Atom_Float*)dB_max_atom)->body;
                    }
                    if(linFreq_atom) {
                        linFreq = ((const LV2_Atom_Float*)linFreq_atom)->body;
                    }
                    if(log_atom) {
                        log = ((const LV2_Atom_Bool*)log_atom)->body != 0;
                    }
                }
            }
            ev = lv2_atom_sequence_next(ev);
        }
    }

    // Process audio data
    for (uint32_t c = 0; c < 2; ++c) {
        if (ui_active) {
            // If UI is active, send raw audio data to UI
            tx_rawaudio((int32_t)c, n_samples, input[c]);
        }
        // If not processing audio in-place, forward audio
        if (input[c] != output[c]) {
            memcpy(output[c], input[c], sizeof(float) * n_samples);
        }
    }

    // Close off sequence
    lv2_atom_forge_pop(&forge, &seq_frame);

}

LV2_State_Status SignalView::state_save(
    LV2_State_Store_Function  store,
    LV2_State_Handle          handle,
    uint32_t                  flags,
    const LV2_Feature* const* features)
{
    store(handle,
          uris->ui_dB_min,
          (void*)&dB_min,
          sizeof(float),
          uris->atom_Float,
          LV2_STATE_IS_POD);

    store(handle,
          uris->ui_dB_max,
          (void*)&dB_max,
          sizeof(float),
          uris->atom_Float,
          LV2_STATE_IS_POD);

    store(handle,
          uris->ui_linFreq,
          (void*)&linFreq,
          sizeof(float),
          uris->atom_Float,
          LV2_STATE_IS_POD);

    store(handle,
          uris->ui_log,
          (void*)&log,
          sizeof(int32_t),
          uris->atom_Bool,
          LV2_STATE_IS_POD);

    return LV2_STATE_SUCCESS;
}

LV2_State_Status SignalView::state_restore(
    LV2_State_Retrieve_Function retrieve,
    LV2_State_Handle            handle,
    uint32_t                    flags,
    const LV2_Feature* const*   features)
{
    size_t   size     = 0;
    uint32_t type     = 0;
    uint32_t valflags = 0;

    const void *dB_min_p =
        retrieve(handle, uris->ui_dB_min, &size, &type, &valflags);
    if(dB_min_p && size==sizeof(float) && type==uris->atom_Float) {
        dB_min = *((const float*)dB_min_p);
        send_settings_to_ui = true;
    }

    const void *dB_max_p =
        retrieve(handle, uris->ui_dB_max, &size, &type, &valflags);
    if(dB_max_p && size==sizeof(float) && type==uris->atom_Float) {
        dB_max = *((const float*)dB_max_p);
        send_settings_to_ui = true;
    }

    const void *linFreq_p =
        retrieve(handle, uris->ui_linFreq, &size, &type, &valflags);
    if(linFreq_p && size==sizeof(float) && type==uris->atom_Float) {
        linFreq = *((const float*)linFreq_p);
        send_settings_to_ui = true;
    }

    const void *log_p =
        retrieve(handle, uris->ui_log, &size, &type, &valflags);
    if(log_p && size==sizeof(int) && type==uris->atom_Bool) {
        log = *((const int*)log_p) != 0;
        send_settings_to_ui = true;
    }

    return LV2_STATE_SUCCESS;
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
    SignalView *sv;
    try {
        sv = new SignalView(descriptor, rate, bundle_path, features);
    }
    catch (...) {
        sv = nullptr;
    }
    return (LV2_Handle)sv;
}

static void connect_port (LV2_Handle instance, uint32_t port, void *data_location)
{
    SignalView* m = (SignalView*) instance;
    if (m) m->connect_port (port, data_location);
}

static void run (LV2_Handle instance, uint32_t sample_count)
{
    SignalView* m = (SignalView*) instance;
    if (m) m->run (sample_count);
}

static void cleanup (LV2_Handle instance)
{
    SignalView* m = (SignalView*) instance;
    if (m) delete m;
}

static LV2_State_Status
state_save(LV2_Handle                instance,
           LV2_State_Store_Function  store,
           LV2_State_Handle          handle,
           uint32_t                  flags,
           const LV2_Feature* const* features)
{
    SignalView* m = (SignalView*) instance;
    if(m) return m->state_save(store, handle, flags, features);
}

static LV2_State_Status
state_restore(LV2_Handle                  instance,
              LV2_State_Retrieve_Function retrieve,
              LV2_State_Handle            handle,
              uint32_t                    flags,
              const LV2_Feature* const*   features)
{
    SignalView* m = (SignalView*) instance;
    if(m) return m->state_restore(retrieve, handle, flags, features);
}

static const void*
extension_data(const char* uri)
{
    static const LV2_State_Interface state = {state_save, state_restore};
    if (!strcmp(uri, LV2_STATE__interface)) {
        return &state;
    }
    return NULL;
}

static const LV2_Descriptor descriptor =
{
    SIGNAL_VIEW_URI,
    instantiate,
    connect_port,
    NULL,
    run,
    NULL,
    cleanup,
    extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
    if(indexx==0){
        return &descriptor;
    } else {
        return nullptr;
    }
}



