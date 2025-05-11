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

#include "TGraph.h"
#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string.h>
#include <stdio.h>

void TGraph::ProgramLoad(void)
{
    const char *vertShaderSrc =
        "#version 460\n"
        "layout(location=0) in float a_x;\n"
        "layout(location=1) in float a_y;\n"
        "layout(location=2) in float a_v;\n"
        "out float value;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection*vec4(a_x,a_y,0.0,1.0);\n"
        "   value = a_v;\n"
        "}\n";

    const char *fragShaderSrc =
        "#version 460\n"
        "layout(location = 0) out vec4 f_color;\n"
        "in float value;\n"
        "uniform vec4 color;\n"
        "void main()\n"
        "{\n"
        "   f_color = vec4(color.rgb*value, color.a);\n"
        "}\n";

    programObject = LoadProgram(vertShaderSrc, fragShaderSrc);
    if (!programObject)
    {
        printf("lgraph.cpp: Error, couldn't load program.\n");
        return;
    }

    colorLocation = glGetUniformLocation(programObject, "color");
    projectionLocation = glGetUniformLocation(programObject, "projection");
}

void TGraph::ProgramDestroy(void)
{
    glDeleteProgram(programObject);
}

TGraph::TGraph(int Nvertices)
    :Nvertices(Nvertices),
    lineWidth0(1.0),
    lineWidth1(3.0),
    ytop(1.0),
    ybottom(-1.0)
{
    ProgramLoad();

    glGenBuffers(1, &xVBO);
    glGenBuffers(1, &yVBO);
    glGenBuffers(1, &vVBO);

    glBindBuffer(GL_ARRAY_BUFFER, xVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Nvertices,
                 NULL, GL_DYNAMIC_DRAW);

    float *xVBOmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                              0, sizeof(float)*Nvertices,
                                              GL_MAP_WRITE_BIT|
                                              GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<Nvertices;i++){
        xVBOmap[i] = ((float)i/(Nvertices-1));
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindBuffer(GL_ARRAY_BUFFER, yVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Nvertices,
                 NULL, GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Nvertices,
                NULL, GL_DYNAMIC_DRAW);
             
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, xVBO);
    glVertexAttribPointer(X_LOC, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(X_LOC);

    glBindBuffer(GL_ARRAY_BUFFER, yVBO);
    glVertexAttribPointer(Y_LOC, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(Y_LOC);

    glBindBuffer(GL_ARRAY_BUFFER, vVBO);
    glVertexAttribPointer(V_LOC, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(V_LOC);

    glBindVertexArray(0);
    
    view_width = 1.0f;

}

TGraph::~TGraph(void)
{
    ProgramDestroy();

    glDeleteBuffers(1, &xVBO);
    glDeleteBuffers(1, &yVBO);
    glDeleteVertexArrays(1, &VAO);
}

void TGraph::SetColors(glm::vec4 &color0, glm::vec4 &color1){
    TGraph::color0 = color0;
    TGraph::color1 = color1;
}

void TGraph::SetLineWidths(float lineWidth0, float lineWidth1)
{
    TGraph::lineWidth0 = lineWidth0;
    TGraph::lineWidth1 = lineWidth1;
}

void TGraph::SetLimits(float ytop, float ybottom)
{
    TGraph::ytop = ytop;
    TGraph::ybottom = ybottom;
}
    
void TGraph::SetViewWidth(float width)
{
    view_width = width;
}

void TGraph::SetX(float *x, int N)
{
    glBindBuffer(GL_ARRAY_BUFFER, xVBO);

    float *xVBOmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                              0, sizeof(float)*N,
                                              GL_MAP_WRITE_BIT|
                                              GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<N;i++){
        xVBOmap[i] = x[i];
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}

void TGraph::SetValue(float *v, int N)
{
    glBindBuffer(GL_ARRAY_BUFFER, vVBO);

    float *vVBOmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                              0, sizeof(float)*N,
                                              GL_MAP_WRITE_BIT|
                                              GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<N;i++){
        vVBOmap[i] = v[i];
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}


void TGraph::Draw(float *y0, int N)
{
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER, yVBO);

    float *yVBOmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                              0, sizeof(float)*N,
                                              GL_MAP_WRITE_BIT|
                                              GL_MAP_INVALIDATE_BUFFER_BIT);
    memcpy(yVBOmap, y0, sizeof(float)*N);

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glUseProgram(programObject);

    glBindVertexArray(VAO);

    float top = ytop;
    float bottom = ybottom;
    float left = 0.0f;
    float right = view_width;
    float near =  1.0f;
    float far = -1.0f;
    glm::mat4 projection = glm::ortho(left, right, bottom, top, near, far);

    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform4fv(colorLocation, 1, glm::value_ptr(color0));
    glLineWidth(lineWidth0);
    glDrawArrays(GL_LINE_STRIP, 0, N);

    glUniform4fv(colorLocation, 1, glm::value_ptr(color1));
    glLineWidth(lineWidth1);
    glDrawArrays(GL_LINE_STRIP, 0, N);

    glBindVertexArray(0);
    glUseProgram(0);
}



