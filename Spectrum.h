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

    Spectrum.h
    Created: 18 Jan 2024 8:50:34pm
    Author:  tim

  ==============================================================================
*/

#pragma once

#include <complex>
#include <fftw3.h>
#include <memory>
#include <iostream>
#include <deque>
#include "TGraph.h"
#include "LGraph.h"
#include "GraphFill.h"
#include "Waterfall.h"
#include "Grid.h"
#include "Semaphore.h"

struct PtrFifo
{
private:
    std::deque<int> fifo;
    Semaphore sem;

public:
    PtrFifo() : sem(1)
    {

    }

    void Push(int i)
    {
        sem.wait();
        fifo.push_back(i);
        sem.post();
    }
    
    int Pop(void)
    {
        sem.wait();
        int r = fifo.front();
        fifo.pop_front();
        sem.post();
        return r;
    }

    int GetNumReady(void)
    {
        return fifo.size();
    }
};

class Spectrum
{
public:
    Spectrum(
        int Nfft,
        double fsamplerate,
        float frame_rate,
        int Ncopy,
        const char* bundle_path);
    ~Spectrum();
    
    void GLInit(void);
    void GLDestroy(void);
    void Render(void);
    void EvaluateSample(float xl, float xr);
    void SetdBLimits(float dB_min, float dB_max);
    void SetWidth(float frequency);
    void SetColors(float hue_left);
    void SetFrequency(bool log=false);
    
private:
    int Nfft;
    int Nfft_draw;
    int Ndx_draw;
    int Npoints;
    int Npoints_p;
    int Ncopy;
    const char* bundle_path;
    int index_last;
    int i_buffer;
    int i_sample;
    int Ncount;
    int count;
    int i_draw_front;
    int i_draw_back;
    bool log;
    bool log_last;
    float alpha_width;
    glm::vec4 time_color_l0;
    glm::vec4 time_color_l1;
    glm::vec4 time_color_r0;
    glm::vec4 time_color_r1;
    glm::vec4 freq_color_l0;
    glm::vec4 freq_color_l1;
    glm::vec4 freq_color_r0;
    glm::vec4 freq_color_r1;
    glm::vec4 fill_color_l;
    glm::vec4 fill_color_r;
    double fsamplerate;
    float frame_rate;
    std::unique_ptr<float[]> x_cyclic_in_l;
    std::unique_ptr<float[]> x_cyclic_in_r;
    std::unique_ptr<std::unique_ptr<float[]>[]> x_draw_l_raw;
    std::unique_ptr<std::unique_ptr<float[]>[]> x_draw_r_raw;
    std::unique_ptr<float[]> dx_draw_raw;
    std::unique_ptr<float[]> x_draw;
    std::unique_ptr<float[]> v_draw;
    std::unique_ptr<std::unique_ptr<float[]>[]> x_in_l;
    std::unique_ptr<std::unique_ptr<float[]>[]> x_in_r;
    std::unique_ptr<double[]> x_fft;
    bool dataReady;
    std::unique_ptr<std::complex<double>[]> X_fft;
    fftw_plan x_plan;
    std::unique_ptr<float[]> X_db_l;
    std::unique_ptr<float[]> X_db_r;
    std::unique_ptr<float[]> x_points;
    std::unique_ptr<float[]> X_db_l_p;
    std::unique_ptr<float[]> X_db_r_p;
    std::unique_ptr<float[]> x_points_p;
    PtrFifo ptrFifo;
    
    std::unique_ptr<LGraph> lgraph;
    std::unique_ptr<TGraph> tgraph;
    std::unique_ptr<GraphFill> fill;
    std::unique_ptr<Waterfall> waterfall;
    std::unique_ptr<Grid> grid;
    
    void ComputeSpectrum(float *x, std::unique_ptr<float[]> &X_db);
    void InitializeFrequency(void);
    void CoalescePoints(int pix_width);
    void ShadeGraph(std::unique_ptr<float[]> &x_raw, int width_pix, int height_pix);
};


