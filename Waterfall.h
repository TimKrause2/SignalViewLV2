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

    Waterfall.h
    Created: 19 Jan 2024 10:11:06am
    Author:  tim

  ==============================================================================
*/

#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <memory>

struct wPixel
{
    unsigned char r;
    unsigned char g;
};

class Waterfall
{
private:
    bool quadsInitialized;
    int  Npoints;
    int  Nlines;
    bool texture_phase;
    int  line;
    float draw_line;
    float lines_per_frame;
    float threshold;
    float view_width;
    float view_height;
    float dB_min;
    float dB_max;
    std::unique_ptr<wPixel[]> pixels;
    GLuint textures[2];
    GLuint current_tex;
    GLuint trailing_tex;
    GLuint x_vbo;
    GLuint tex_vbo;
    GLuint y_vbos[2];
    GLuint vaos[2];
    GLuint program;
    GLint mvp_loc;
    GLint s_texture_loc;
    GLint color_l_loc;
    GLint color_r_loc;
    void InitQuads(void);
    void DeleteQuads(void);
    unsigned char dB2intensity(float dB);
    void InitializeBuffers(void);
public:
    Waterfall(int Npoints, int Nlines, float line_rate, float frame_rate);
    ~Waterfall();
    
    void InitializeFrequency(bool log=false);
    void SetViewWidth(float width);
    void SetViewHeight(float height);
    void SetdBLimits(float dB_min, float dB_max);
    void InsertLine(float *data_l, float *data_r);
    void Render(glm::vec4 &color_l, glm::vec4 &color_r);
};

struct Attributes
{
    glm::vec2 vertex;
    glm::vec2 texel;
};

struct Pixel
{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};



