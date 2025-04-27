/*
  ==============================================================================

    Grid.h
    Created: 23 Oct 2024 3:13:01pm
    Author:  tim

    Border
    4 vertices

    Magnitude dB lines 0dB to -180dB
    8 lines - 24dB
    16 lines - 12dB
    31 lines - 6dB
    61 lines - 3dB
    116 lines - total

    Linear view frequency lines
    9 lines - 100k range 10k lines
    9 lines - 50k range 5k lines
    9 lines - 20k range 2k lines
    9 lines - 10k range 1k lines
    9 lines - 5k range  0.5k lines
    9 lines - 2k range  0.2k lines
    9 lines - 1k range  0.1k lines
    63 lines - total

    Log view frequency lines
    4 lines - decade markers
    3*8 + 1 = 25 lines - linear markers
    29 lines - total


  ==============================================================================
*/

#pragma once
#define _USE_MATH_DEFINES

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "Font.h"

#define N_LOG_DECADE 4
#define N_LOG_LINEAR 25
#define N_LOG_TOTAL  29
#define N_DB_24 8
#define N_DB_12 16
#define N_DB_6 31
#define N_DB_3 61
#define N_DB_TOTAL 116
#define DB_OFFSET_24 0
#define DB_OFFSET_12 N_DB_24
#define DB_OFFSET_6 (DB_OFFSET_12 + N_DB_12)
#define DB_OFFSET_3 (DB_OFFSET_6 + N_DB_6)
#define N_LINEAR_TOTAL 63

struct LinearRange
{
    float frequency;
    int   offset;
    float freq_per_line;
};

class Grid
{
    GLuint program;
    GLint color_loc;
    GLint mvp_loc;
    GLuint border_vbo;
    GLuint log_vbo;
    GLuint dB_vbo;
    GLuint linear_vbo;
    GLuint border_vao;
    GLuint log_vao;
    GLuint dB_vao;
    GLuint linear_vao;
    GLint viewport[4];
    int   Nfft;
    bool  log;
    float dB_top;
    float dB_bottom;
    float view_width;
    float sample_rate;
    glm::vec4 log_border_color;
    glm::vec4 log_decade_color;
    glm::vec4 log_linear_color;
    glm::vec4 dB_color;
    FreeTypeFont font;
    FreeTypeFont dB_font;
    int font_height;
    char font_path[1024];

    void ProgramLoad(void);
    void ProgramDestroy(void);

    void InitializeBorder(void);
    void InitializeLinear(void);
    void InitializeLog(void);
    void InitializedB(void);

    float x_LogDisplacement(float frequency);
    float x_LinearDisplacement(float frequency);

    void DrawLogFrequencyText(float frequency, const char *text);

    float y_dBDisplacement(float dB);

    void DrawBorder(void);
    void Draw_dBText(float dB);
    void Draw_dB();
    void DrawLogFrequency(void);
    void DrawLinearFrequency(void);

public:
    Grid(int Nfft, float sample_rate, const char* bundle_path);
    ~Grid(void);
    void SetFrequency(bool log=false);
    void SetLimits(float dB_top, float dB_bottom);
    void SetViewWidth(float view_width);
    void Draw(void);
};
