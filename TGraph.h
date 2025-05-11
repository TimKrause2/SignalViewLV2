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

#pragma once
#define _USE_MATH_DEFINES

#include <glad/gl.h>
#include <glm/glm.hpp>

#define X_LOC 0
#define Y_LOC 1
#define V_LOC 2

class TGraph
{
    GLuint programObject;
    GLint  colorLocation;
    GLint  projectionLocation;
    GLuint xVBO;
    GLuint yVBO;
    GLuint vVBO;
    GLuint VAO;
    int Nvertices;
    glm::vec4 color0;
    glm::vec4 color1;
    float lineWidth0;
    float lineWidth1;
    float ytop;
    float ybottom;
    float view_width;

    void ProgramLoad(void);
    void ProgramDestroy(void);

public:
    TGraph(int Nvertices);
    ~TGraph(void);
    void SetColors(glm::vec4 &color0, glm::vec4 &color1);
    void SetLineWidths(float lineWidth0, float lineWidth1);
    void SetLimits(float ytop, float ybottom);
    void SetViewWidth(float width);
    void SetX(float *x, int N);
    void SetValue(float *v, int N);
    void Draw(float *y, int N);
};

