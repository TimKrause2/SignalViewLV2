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
    Waterfall(int Npoints, int Nlines);
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



