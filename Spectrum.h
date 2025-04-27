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
#include "LGraph.h"
#include "Waterfall.h"
#include "Grid.h"

struct PtrFifo
{
private:
    std::deque<int> fifo;

public:
    void Push(int i)
    {
        fifo.push_back(i);
    }
    
    int Pop(void)
    {
        int r = fifo.front();
        fifo.pop_front();
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
    int Npoints;
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
    glm::vec4 time_color_l0;
    glm::vec4 time_color_l1;
    glm::vec4 time_color_r0;
    glm::vec4 time_color_r1;
    glm::vec4 freq_color_l0;
    glm::vec4 freq_color_l1;
    glm::vec4 freq_color_r0;
    glm::vec4 freq_color_r1;
    double fsamplerate;
    std::unique_ptr<float[]> x_cyclic_in_l;
    std::unique_ptr<float[]> x_cyclic_in_r;
    std::unique_ptr<std::unique_ptr<float[]>[]> x_draw_l;
    std::unique_ptr<std::unique_ptr<float[]>[]> x_draw_r;
    std::unique_ptr<std::unique_ptr<float[]>[]> x_in_l;
    std::unique_ptr<std::unique_ptr<float[]>[]> x_in_r;
    std::unique_ptr<double[]> x_fft;
    bool dataReady;
    std::unique_ptr<std::complex<double>[]> X_fft;
    fftw_plan x_plan;
    std::unique_ptr<float[]> X_db_l;
    std::unique_ptr<float[]> X_db_r;
    PtrFifo ptrFifo;
    
    std::unique_ptr<LGraph> lgraph;
    std::unique_ptr<LGraph> tgraph;
    std::unique_ptr<Waterfall> waterfall;
    std::unique_ptr<Grid> grid;
    
    void ComputeSpectrum(float *x, std::unique_ptr<float[]> &X_db);
    void InitializeFrequency(void);
};


