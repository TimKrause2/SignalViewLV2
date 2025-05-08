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

/*
  ==============================================================================

    Spectrum.cpp
    Created: 18 Jan 2024 8:50:34pm
    Author:  tim

  ==============================================================================
*/

#define GLM_ENABLE_EXPERIMENTAL
#include "Spectrum.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <glm/gtx/color_space.hpp>

Spectrum::Spectrum(
    int Nfft,
    double fsamplerate,
    float frame_rate,
    int Ncopy,
    const char* bundle_path)
    :
    Nfft(Nfft),
    fsamplerate(fsamplerate),
    frame_rate(frame_rate),
    Ncopy(Ncopy),
    bundle_path(bundle_path)
{
    Npoints = Nfft/2 + 1;
    x_fft.reset(new double[Nfft]);
    X_fft.reset(new std::complex<double>[Npoints]);
    X_db_l.reset(new float[Npoints]);
    X_db_r.reset(new float[Npoints]);
    x_points.reset(new float[Npoints]);
    X_db_l_p.reset(new float[Npoints]);
    X_db_r_p.reset(new float[Npoints]);
    x_points_p.reset(new float[Npoints]);
    x_plan = fftw_plan_dft_r2c_1d(
        Nfft,
        x_fft.get(),
        reinterpret_cast<fftw_complex*>(X_fft.get()),
        FFTW_MEASURE);
    dataReady = false;
    x_cyclic_in_l.reset(new float[Nfft]);
    x_cyclic_in_r.reset(new float[Nfft]);
    x_draw_l.reset(new std::unique_ptr<float[]>[2]);
    x_draw_r.reset(new std::unique_ptr<float[]>[2]);
    x_draw_l[0].reset(new float[Nfft]);
    x_draw_l[1].reset(new float[Nfft]);
    x_draw_r[0].reset(new float[Nfft]);
    x_draw_r[1].reset(new float[Nfft]);
    x_in_l.reset(new std::unique_ptr<float[]>[Ncopy]);
    x_in_r.reset(new std::unique_ptr<float[]>[Ncopy]);
    for(int c=0;c<Ncopy;c++){
        x_in_l[c].reset(new float[Nfft]);
        x_in_r[c].reset(new float[Nfft]);
    }
    SetColors(30.0f);
    index_last=0;
    i_buffer = 0;
    i_sample = 0;
    Ncount = Nfft/Ncopy;
    count = Ncount;
    i_draw_front = 0;
    i_draw_back = 1;
    log = false;
    log_last = false;
    for(int i=0;i<Nfft;i++){
        x_draw_l[i_draw_front][i] = 0.0f;
        x_draw_r[i_draw_front][i] = 0.0f;
    }
}
    
Spectrum::~Spectrum()
{
}

void Spectrum::GLInit(void)
{
    tgraph.reset(new LGraph(Nfft));
    tgraph->SetLineWidths(3.0f, 1.0f);
    tgraph->SetLimits(1.0f, -1.0f);
    
    lgraph.reset(new LGraph(Npoints));
    lgraph->SetLineWidths( 3.0f, 1.0f );
    lgraph->SetLimits(0.0f, -180.0f);
    
    fill.reset(new GraphFill(Npoints));
    fill->SetLimits(0.0f, -180.0f);

    float line_rate = fsamplerate/Nfft*Ncopy;
    waterfall.reset(new Waterfall(Npoints, 128, line_rate, frame_rate));

    grid.reset(new Grid(Nfft, fsamplerate, bundle_path));

    InitializeFrequency();
}

void Spectrum::GLDestroy(void)
{
    lgraph.reset(nullptr);
    tgraph.reset(nullptr);
    fill.reset(nullptr);
    waterfall.reset(nullptr);
    grid.reset(nullptr);
}

double window_func(double alpha)
{
    if(alpha>0.5)
        return 0.0;
    if(alpha<-0.5)
        return 0.0;
    return 0.53836 + 0.46164*cos(2.0*M_PI*(alpha-0.5));
}

double Blackman_Harris_window_func(double alpha)
{
    double a0 = 0.35875;
    double a1 = 0.48829;
    double a2 = 0.14128;
    double a3 = 0.01168;
    return a0 - a1*cos(2*M_PI*alpha) + a2*cos(4*M_PI*alpha)
           - a3*cos(6*M_PI*alpha);
}

void Spectrum::ComputeSpectrum(float *x, std::unique_ptr<float[]> &X_db)
{
    for(int i=0;i<Nfft;i++){
        double alpha = (double)i/Nfft;
        x_fft[i] = x[i]*Blackman_Harris_window_func(alpha);
    }
    fftw_execute( x_plan );
    float norm_fact = 2.0f/0.3587500f/Nfft;
    for(int i=0;i<Npoints;i++){
        float abs_X = (float)abs(X_fft[i])*norm_fact;
        //if(i) abs_X*=(float)i*100.0f/Npoints;
        if(abs_X < 1e-9) abs_X = 1e-9;
        X_db[i] = 20.0f * log10f(abs_X);
    }
}

void Spectrum::Render(void)
{
    if(log!=log_last){
        InitializeFrequency();
        log_last = log;
    }

    int n = Ncopy;
    float *x_l=nullptr;
    float *x_r=nullptr;
    while(ptrFifo.GetNumReady()>0 && n!=0){
        index_last = ptrFifo.Pop();
        x_l = x_in_l[index_last].get();
        x_r = x_in_r[index_last].get();
        ComputeSpectrum(x_l, X_db_l);
        ComputeSpectrum(x_r, X_db_r);
        waterfall->InsertLine(X_db_l.get(), X_db_r.get());
        n--;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 2*viewport[3]/3, viewport[2], viewport[3]/3);
    tgraph->SetColors(time_color_l0, time_color_l1);
    tgraph->Draw(x_draw_l[i_draw_front].get(), Nfft);
    tgraph->SetColors(time_color_r0, time_color_r1);
    tgraph->Draw(x_draw_r[i_draw_front].get(), Nfft);
    
    glViewport(0, viewport[3]/3, viewport[2], viewport[3]/3);
    grid->Draw();
    CoalescePoints(viewport[2]);
    lgraph->SetX(x_points_p.get(), Npoints_p);
    fill->SetX(x_points_p.get(), Npoints_p);
    lgraph->SetColors(freq_color_l0, freq_color_l1);
    lgraph->Draw(X_db_l_p.get(), Npoints_p);
    lgraph->SetColors(freq_color_r0, freq_color_r1);
    lgraph->Draw(X_db_r_p.get(), Npoints_p);
    fill->SetColor(fill_color_l);
    fill->Draw(X_db_l_p.get(), Npoints_p);
    fill->SetColor(fill_color_r);
    fill->Draw(X_db_r_p.get(), Npoints_p);

    glDisable(GL_BLEND);
    
    glViewport(0, 0, viewport[2], viewport[3]/3);
    waterfall->Render(time_color_l1, time_color_r1);
    //std::cout << ".";
    //std::cout.flush();
}

void Spectrum::SetdBLimits(float dB_min, float dB_max)
{
    if(fill)
        fill->SetLimits(dB_max, dB_min);
    if(lgraph)
        lgraph->SetLimits(dB_max, dB_min);
    if(waterfall)
        waterfall->SetdBLimits(dB_min, dB_max);
    if(grid)
        grid->SetLimits(dB_max, dB_min);
}

void Spectrum::SetWidth(float frequency)
{
    alpha_width = frequency/(fsamplerate/2.0);
    if(fill)
        fill->SetViewWidth(alpha_width);
    if(lgraph)
        lgraph->SetViewWidth(alpha_width);
    if(waterfall)
        waterfall->SetViewWidth(alpha_width);
    if(grid)
        grid->SetViewWidth(alpha_width);
}

void Spectrum::EvaluateSample(float x_l, float x_r)
{
    x_cyclic_in_l[i_sample] = x_l;
    x_cyclic_in_r[i_sample] = x_r;
    i_sample++;
    if(i_sample==Nfft)
    {
        i_sample = 0;
        for(int i=0;i<Nfft;i++){
            x_draw_l[i_draw_back][i] = x_cyclic_in_l[i];
            x_draw_r[i_draw_back][i] = x_cyclic_in_r[i];
        }
        i_draw_front ^= 1;
        i_draw_back ^= 1;
    }
    
    if(--count==0){
        count = Ncount;
        if(ptrFifo.GetNumReady()<Ncopy){
            int N1 = Nfft - i_sample;
            int N2 = Nfft - N1;
            int i_src = i_sample;
            int i_dst = 0;
            for(int i=0;i<N1;i++){
                x_in_l[i_buffer][i_dst] = x_cyclic_in_l[i_src];
                x_in_r[i_buffer][i_dst] = x_cyclic_in_r[i_src];
                i_src++;
                i_dst++;
            }
            i_src = 0;
            for(int i=0;i<N2;i++){
                x_in_l[i_buffer][i_dst] = x_cyclic_in_l[i_src];
                x_in_r[i_buffer][i_dst] = x_cyclic_in_r[i_src];
                i_src++;
                i_dst++;
            }
            ptrFifo.Push(i_buffer);
            i_buffer++;
            if(i_buffer==Ncopy)
                i_buffer=0;
        }
    }
}

glm::vec4 hsv2rgba(float hue, float sat, float val, float alpha)
{
    glm::vec3 hsv(hue, sat, val);
    glm::vec3 rgb = glm::rgbColor(hsv);
    return glm::vec4(rgb, alpha);
}

void Spectrum::SetColors(float hue_l)
{
    float hue_r = hue_l + 180.0f;
    if(hue_r>=360.0f)
        hue_r -= 360.0f;
    time_color_l0 = hsv2rgba(hue_l, 1.0f, 0.125f, 1.0f);
    time_color_l1 = hsv2rgba(hue_l, 1.0f, 1.0f, 1.0f);
    time_color_r0 = hsv2rgba(hue_r, 1.0f, 0.125f, 1.0f);
    time_color_r1 = hsv2rgba(hue_r, 1.0f, 1.0f, 1.0f);
    freq_color_l0 = hsv2rgba(hue_l, 1.0f, 0.25f, 1.0f);
    freq_color_l1 = hsv2rgba(hue_l, 1.0f, 0.5f, 1.0f);
    freq_color_r0 = hsv2rgba(hue_r, 1.0f, 0.25f, 1.0f);
    freq_color_r1 = hsv2rgba(hue_r, 1.0f, 0.5f, 1.0f);
    fill_color_l = hsv2rgba(hue_l, 1.0f, 0.5f, 1.0f);
    fill_color_r = hsv2rgba(hue_r, 1.0f, 0.5f, 1.0f);
}

void Spectrum::SetFrequency(bool log)
{
    Spectrum::log = log;
}

void Spectrum::InitializeFrequency(void)
{
    if(!log){
        for(int i=0;i<Npoints;i++){
            float alpha = (float)i/(Npoints-1);
            x_points[i] = alpha;
        }
    }else{
        x_points[0] = 0.0f;
        float alpha2 = logf(2.0f)/logf((float)Npoints);
        float beta = alpha2/(1.0f + alpha2);
        float one_m_beta = 1.0f - beta;

        for(int i=1;i<Npoints;i++){
            float alpha = logf((float)i) / logf((float)Npoints);
            float f = beta + alpha*one_m_beta;
            x_points[i] = f;
        }
    }
    //fill->SetX(x.get());
    //lgraph->SetX(x.get());
    waterfall->InitializeFrequency(log);
    grid->SetFrequency(log);
}

void Spectrum::CoalescePoints(int pix_width)
{
    float db_max_l = X_db_l[0];
    float db_max_r = X_db_r[0];
    int i_p=0;
    float pix_threshold = floorf(x_points[0]*pix_width + 1.0f);
    float alpha0 = 0.0f;
    for(int i=1;i<Npoints;i++){
        float pix = x_points[i]*pix_width/alpha_width;
        if(pix>=pix_threshold 
            || i==(Npoints-1)
            || pix >= pix_width)
        {
            x_points_p[i_p] = alpha0;
            X_db_l_p[i_p] = db_max_l;
            X_db_r_p[i_p] = db_max_r;
            db_max_l = X_db_l[i];
            db_max_r = X_db_r[i];
            i_p++;
            alpha0 = x_points[i];
            if(pix>=pix_width)
                break;
            pix_threshold = floorf(pix+1.0f);
        } else {
            if(X_db_l[i]>db_max_l){
                db_max_l = X_db_l[i];
            }
            if(X_db_r[i]>db_max_r){
                db_max_r = X_db_r[i];
            }
        }
    }
    Npoints_p = i_p;
}