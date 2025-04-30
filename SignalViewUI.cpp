#include "SignalViewUI.h"

SignalViewUI::SignalViewUI(
    const LV2UI_Descriptor *descriptor,
    const char *plugin_uri,
    const char *bundle_path,
    LV2UI_Write_Function write_function,
    LV2UI_Controller controller,
    LV2UI_Widget *widget,
    const LV2_Feature *const *features)
    :
    write(write_function),
    controller(controller),
    bundle_path(bundle_path)
{
    parentXWindow = nullptr;
    map = nullptr;
    logger_log = nullptr;

    const char* missing = lv2_features_query(
        features,
        LV2_URID__map, &map, true,
        LV2_UI__parent, &parentXWindow, true,
        LV2_LOG__log, &logger_log, false,
        NULL);

    if (missing) {
        printf("Missing feature <%s>\n", missing);
        throw;
    }

    uris = new SignalViewURIs(map);
    lv2_atom_forge_init(&forge, map);
    lv2_log_logger_init(&logger, map, logger_log);

    lv2_log_note(&logger, "SignalViewUI::SignalViewUI logger initialized.\n");

    state_valid = false;
    quit = false;
    rate = 48000.0f;
    dB_min = -180.0f;
    dB_max = 0.0f;
    linFreq = rate/2.0f;
    log = false;
    mousing = false;

    send_ui_send_state();

    std::function<void()> deferred_task = std::bind(ui_thread_func, this);

    ui_thread = std::thread(deferred_task);

}

SignalViewUI::~SignalViewUI()
{
    quit = true;

    ui_thread.join();
}

void ui_thread_func(SignalViewUI* ui)
{
    // Initialize the pugl Window and View
    ui->setupPugl();

    // Enter the event loop
    ui->eventLoop();

}

void SignalViewUI::eventLoop(void)
{
    if(!view_ready){
        puglFreeView(view);
        puglFreeWorld(world);
        return;
    }

    // wait for the state
    state_sem.wait();

    // initialize the spectrum with the new state
    setSpectrum();

    // enter the event loop
    while(!quit)
    {
        puglUpdate(world, -1.0);
    }
    printf("SignalViewUI::eventLoop puglFreeView\n");
    puglFreeView(view);
    puglFreeWorld(world);
}

void SignalViewUI::setupPugl(void)
{
    world = puglNewWorld(PUGL_MODULE, 0);
    view = puglNewView(world);
    puglSetWorldString(world, PUGL_CLASS_NAME, "SignalView");
    puglSetViewString(view, PUGL_WINDOW_TITLE, "SignalView");
    puglSetSizeHint(view, PUGL_DEFAULT_SIZE, 400, 400);
    puglSetSizeHint(view, PUGL_MIN_SIZE, 200, 200);
    puglSetSizeHint(view, PUGL_MAX_SIZE, 1920, 1080);
    puglSetSizeHint(view, PUGL_MIN_ASPECT, 1, 1);
    puglSetSizeHint(view, PUGL_MAX_ASPECT, 16, 9);
    puglSetBackend(view, puglGlBackend());
    puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_API);
    puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 4);
    puglSetViewHint(view, PUGL_CONTEXT_VERSION_MINOR, 6);
    puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
    puglSetViewHint(view, PUGL_DOUBLE_BUFFER, PUGL_TRUE);
    puglSetViewHint(view, PUGL_RESIZABLE, PUGL_TRUE);
    puglSetViewHint(view, PUGL_SWAP_INTERVAL, PUGL_TRUE);
    puglSetHandle(view, this);
    puglSetParent(view, (PuglNativeView)parentXWindow);
    puglSetEventFunc(view, ::onEvent);

    const PuglStatus st = puglRealize(view);
    if (st) {
        lv2_log_error(
            &logger,
            "SignalViewUI::setupPugl puglRealize failed:%s\n",
        puglStrerror(st));
        view_ready = false;
        view_sem.post();
    }else{
        lv2_log_note(&logger, "SignalViewUI::setupPugl view realized.\n");

        view_ready = true;

        // Show window
        puglShow(view, PUGL_SHOW_RAISE);

        // Signal to the host that the view is ready
        view_sem.post();
    }
}

void SignalViewUI::setSpectrum(void)
{
    if(spectrum){
        spectrum->SetdBLimits(dB_min, dB_max);
        spectrum->SetWidth(linFreq);
        spectrum->SetFrequency(log);
    }
}

void SignalViewUI::setupGL(void)
{
    // load glad
    if(!gladLoadGL((GLADloadfunc)&puglGetProcAddress)){
        lv2_log_error(&logger, "SignalViewUI::setupGL gladLoadGL failed.\n");
        return;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Create a new SignalViewGL
    spectrum.reset(new Spectrum((int)(rate/10.0f),rate,2,bundle_path));
    spectrum->GLInit();
    setSpectrum();

    // enable data from the plugin
    send_ui_enable();
}

void SignalViewUI::teardownGL(void)
{
    // disable data form the plugin
    send_ui_disable();

    // destroy the SignalViewGL
    spectrum.reset(nullptr);
}

void SignalViewUI::onConfigure(int width, int height)
{
    glViewport(0, 0, width, height);
    SignalViewUI::width = width;
    SignalViewUI::height = height;
    h1 = height/3;
    h2 = height*2/3;
}

void SignalViewUI::onExpose(void)
{
    glViewport(0, 0, width, height);
    // draw the SignalViewGL
    // printf("SignalViewUI::onExpose\n");
    if(spectrum) spectrum->Render();
}

void SignalViewUI::onScroll(int y, int dy)
{
    if(y>=h1 && y<=h2){
        float delta = dy * 2;
        float alpha = (float)(y-h1)/(float)(h2-h1);
        float d_dB_min = delta*(1.0f - alpha);
        float d_dB_max = -delta*alpha;
        dB_min += d_dB_min;
        dB_max += d_dB_max;
        if(dB_min < -180.0f) dB_min = -180.0f;
        if(dB_min > 0.0f) dB_min = 0.0f;
        if(dB_max < -180.0f) dB_max = -180.0f;
        if(dB_max > 0.0f) dB_max = 0.0f;
        if(dB_max < dB_min){
            float t = dB_min;
            dB_min = dB_max;
            dB_max = t;
        }
        if(spectrum) spectrum->SetdBLimits(dB_min, dB_max);
        send_ui_state();
    }
}

void SignalViewUI::onButtonPress(const PuglButtonEvent* e)
{
    int button = e->button;
    if(button==BUTTON_LOG){
        log = !log;
        if(spectrum) spectrum->SetFrequency(log);
        if(log){
            linFreq = rate/2.0f;
            if(spectrum) spectrum->SetWidth(linFreq);
        }
        send_ui_state();
    }else if(button==BUTTON_MOTION){
        mousing = true;
        x_last = e->x;
        y_last = e->y;
    }
}

void SignalViewUI::onButtonRelease(const PuglButtonEvent* e)
{
    int button = e->button;
    if(button==BUTTON_MOTION){
        mousing = false;
    }
}

void SignalViewUI::onMotion(const PuglMotionEvent* e)
{
    if(mousing && !log){
        float dx = e->x - x_last;
        linFreq -= dx * 50.0f;
        if(linFreq<1000.0f)linFreq = 1000.0f;
        float nyquist = rate/2.0;
        if(linFreq>nyquist)linFreq = nyquist;
        if(spectrum) spectrum->SetWidth(linFreq);
        send_ui_state();
        x_last = e->x;
        y_last = e->y;
    }
}

void SignalViewUI::port_event(
    uint32_t port_index,
    uint32_t buffer_size,
    uint32_t format,
    const void *buffer)
{
    if(format == uris->atom_eventTransfer){
        const LV2_Atom* atom = (const LV2_Atom*)buffer;
        if(lv2_atom_forge_is_object_type(&forge, atom->type)){
            const LV2_Atom_Object* obj = (const LV2_Atom_Object*)atom;
            if(obj->body.otype == uris->RawAudio){
                recv_raw_audio(obj);
            }else if(obj->body.otype == uris->ui_State){
                recv_ui_state(obj);
            }
        }
    }
}



PuglStatus onEvent(PuglView* view, const PuglEvent* event)
{
    SignalViewUI* sv = (SignalViewUI*)puglGetHandle(view);
    if(sv){
        return sv->onEvent(event);
    }
    return PUGL_SUCCESS;
}

PuglStatus SignalViewUI::onEvent(const PuglEvent* event)
{
    switch (event->type)
    {
    case PUGL_REALIZE:
        printf("PUGL_REALIZE\n");
        setupGL();
        break;
    case PUGL_UNREALIZE:
        printf("PUGLE_UNREALIZE\n");
        teardownGL();
        break;
    case PUGL_CONFIGURE:
        printf("PUGL_CONFIGURE w:%d h:%d\n",
            event->configure.width,
            event->configure.height);
        onConfigure(event->configure.width, event->configure.height);
        break;
    case PUGL_UPDATE:
        //printf("PUGL_UPDATE\n");
        puglObscureView(view);
        break;
    case PUGL_EXPOSE:
        //printf("PUGL_EXPOSE\n");
        onExpose();
        break;
    case PUGL_CLOSE:
        printf("PUGL_CLOSE\n");
        quit = true;
        break;
    case PUGL_KEY_PRESS:
        printf("PUGL_KEY_PRESS\n");
        if (event->key.key == 'q' || event->key.key == PUGL_KEY_ESCAPE)
        {
            quit = true;
        }
        break;
    case PUGL_SCROLL:
        onScroll(event->scroll.y, event->scroll.dy);
        break;
    case PUGL_BUTTON_PRESS:
        onButtonPress(&event->button);
        break;
    case PUGL_BUTTON_RELEASE:
        onButtonRelease(&event->button);
        break;
    case PUGL_MOTION:
        onMotion(&event->motion);
        break;
    default:
        break;
    }

    return PUGL_SUCCESS;
}

PuglNativeView SignalViewUI::getNativeView(void)
{
    // wait for the view to be initialized;
    view_sem.wait();

    if(view_ready){
        // return the native view
        return puglGetNativeView(view);
    }else{
        return (PuglNativeView)0;
    }

}

int SignalViewUI::ui_idle(void)
{
    if(state_valid){
        puglUpdate(world, 0.0);
    }
    if(quit)
        return 1;
    else
        return 0;
}

void SignalViewUI::send_ui_state(void)
{
    lv2_atom_forge_set_buffer(&forge, obj_buf, sizeof(obj_buf));
    LV2_Atom_Forge_Frame frame;
    LV2_Atom*            msg =
      (LV2_Atom*)lv2_atom_forge_object(&forge, &frame, 0, uris->ui_State);
  
    assert(msg);
  
    lv2_atom_forge_key(&forge, uris->ui_dB_min);
    lv2_atom_forge_float(&forge, dB_min);

    lv2_atom_forge_key(&forge, uris->ui_dB_max);
    lv2_atom_forge_float(&forge, dB_max);

    lv2_atom_forge_key(&forge, uris->ui_linFreq);
    lv2_atom_forge_float(&forge, linFreq);

    lv2_atom_forge_key(&forge, uris->ui_log);
    lv2_atom_forge_bool(&forge, log);

    lv2_atom_forge_pop(&forge, &frame);

    write(
        controller,
        0,
        lv2_atom_total_size(msg),
        uris->atom_eventTransfer,
        msg);
}

void SignalViewUI::send_ui_disable(void)
{
    lv2_atom_forge_set_buffer(&forge, obj_buf, sizeof(obj_buf));

    LV2_Atom_Forge_Frame frame;
    LV2_Atom*            msg =
      (LV2_Atom*)lv2_atom_forge_object(&forge, &frame, 0, uris->ui_Off);
  
    assert(msg);
  
    lv2_atom_forge_pop(&forge, &frame);
    write(controller,
              0,
              lv2_atom_total_size(msg),
              uris->atom_eventTransfer,
              msg);
  
}

void SignalViewUI::send_ui_enable(void)
{
    lv2_atom_forge_set_buffer(&forge, obj_buf, sizeof(obj_buf));

    LV2_Atom_Forge_Frame frame;
    LV2_Atom*            msg =
      (LV2_Atom*)lv2_atom_forge_object(&forge, &frame, 0, uris->ui_On);
  
    assert(msg);
  
    lv2_atom_forge_pop(&forge, &frame);
    write(controller,
              0,
              lv2_atom_total_size(msg),
              uris->atom_eventTransfer,
              msg);
  
}

void SignalViewUI::send_ui_send_state(void)
{
    lv2_atom_forge_set_buffer(&forge, obj_buf, sizeof(obj_buf));

    LV2_Atom_Forge_Frame frame;
    LV2_Atom*            msg =
      (LV2_Atom*)lv2_atom_forge_object(&forge, &frame, 0, uris->ui_SendState);
  
    assert(msg);
  
    lv2_atom_forge_pop(&forge, &frame);
    write(controller,
              0,
              lv2_atom_total_size(msg),
              uris->atom_eventTransfer,
              msg);
}

void SignalViewUI::recv_raw_audio(const LV2_Atom_Object* obj)
{
    const LV2_Atom* nChannels_atom = NULL;
    const LV2_Atom* data_atom = NULL;
    const int n_props = lv2_atom_object_get(
        obj,
        uris->nChannels, &nChannels_atom,
        uris->audioData, &data_atom,
        0);
    
    if(n_props!=2 || nChannels_atom->type!=uris->atom_Int
    || data_atom->type!=uris->atom_Vector){
        return;
    }
    const int nChannels = ((const LV2_Atom_Int*)nChannels_atom)->body;
    if(nChannels!=2){
        return;
    }
    const LV2_Atom_Vector* vec = (const LV2_Atom_Vector*)data_atom;
    if(vec->body.child_type != uris->atom_Float){
        return;
    }
    const size_t n_elem =
        (data_atom->size - sizeof(LV2_Atom_Vector_Body))/sizeof(float)/nChannels;

    const float* data = (const float*)(&vec->body+1);
    if(spectrum){
        for(size_t i=0;i<n_elem;i++){
            float l = *(data++);
            float r = *(data++);
            spectrum->EvaluateSample(l, r);
        }
    }
}

void SignalViewUI::recv_ui_state(const LV2_Atom_Object* obj)
{
    const LV2_Atom* dB_min_atom = NULL;
    const LV2_Atom* dB_max_atom = NULL;
    const LV2_Atom* linFreq_atom = NULL;
    const LV2_Atom* log_atom = NULL;
    const LV2_Atom* rate_atom = NULL;
    lv2_atom_object_get(
        obj,
        uris->ui_dB_min, &dB_min_atom,
        uris->ui_dB_max, &dB_max_atom,
        uris->ui_linFreq, &linFreq_atom,
        uris->ui_log, &log_atom,
        uris->param_sampleRate, &rate_atom,
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
    if(rate_atom) {
        rate = ((const LV2_Atom_Float*)rate_atom)->body;

        // signal to the ui thread that the state is valid
        state_sem.post();
    }
}

static LV2UI_Handle instantiate(const struct LV2UI_Descriptor *descriptor, const char *plugin_uri, const char *bundle_path, LV2UI_Write_Function write_function, LV2UI_Controller controller, LV2UI_Widget *widget, const LV2_Feature *const *features)
{
    printf("instantiate\n");
    if (strcmp (plugin_uri, SIGNAL_VIEW_URI) != 0) return nullptr;

    SignalViewUI* ui;
    try
    {
        ui = new SignalViewUI(
            descriptor,
            plugin_uri,
            bundle_path,
            write_function,
            controller,
            widget,
            features);
    }
    catch(...)
    {
        return nullptr;
    }

    PuglNativeView nview = ui->getNativeView();
    if(nview){
        *widget = (LV2UI_Widget)nview;
        return (LV2UI_Handle) ui;
    }else{
        delete ui;
        return (LV2UI_Handle)nullptr;
    }
}

static void cleanup (LV2UI_Handle ui)
{
    printf("cleanup\n");
    SignalViewUI* sui = static_cast<SignalViewUI*>(ui);
    if(sui) delete sui;
}

static void port_event(
    LV2UI_Handle ui,
    uint32_t port_index,
    uint32_t buffer_size,
    uint32_t format,
    const void *buffer)
{
    SignalViewUI* sui = static_cast<SignalViewUI*>(ui);
    if(sui)
        sui->port_event(port_index,buffer_size,format,buffer);
}

static const void * extension_data (const char *uri)
{
    return nullptr;
}

static const LV2UI_Descriptor ui_descriptor =
{
    SIGNAL_VIEW_UI_URI,
    instantiate,
    cleanup,
    port_event,
    extension_data
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor * 	lv2ui_descriptor (uint32_t index)
{
    switch (index)
    {
        case 0:     return &ui_descriptor;
        default:    return 0;
    }
}

