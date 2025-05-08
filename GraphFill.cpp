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

#include "GraphFill.h"
#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string.h>
#include <stdio.h>

void GraphFill::ProgramLoad(void)
{
    const char *vertShaderSrc =
        "#version 460\n"
        "layout(location=0) in float a_x;\n"
        "layout(location=1) in float a_y;\n"
        "layout(location=2) in float a_value;\n"
        "out float value;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection*vec4(a_x,a_y,0.0,1.0);\n"
        "   value = a_value;\n"
        "}\n";

    const char *fragShaderSrc =
        "#version 460\n"
        "layout(location = 0) out vec4 f_color;\n"
        "uniform vec4 color;\n"
        "in float value;\n"
        "void main()\n"
        "{\n"
        "   f_color = vec4(color.rgb*value, 1.0);\n"
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

void GraphFill::ProgramDestroy(void)
{
    glDeleteProgram(programObject);
}

GraphFill::GraphFill(int Nvertices)
    :Nvertices(Nvertices),
    ytop(1.0f),
    ybottom(-1.0f)
{
    ProgramLoad();

    glGenBuffers(1, &xVBO);
    glGenBuffers(1, &yVBO);
    glGenBuffers(1, &vVBO);

    glBindBuffer(GL_ARRAY_BUFFER, xVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Nvertices*2,
                 NULL, GL_DYNAMIC_DRAW);

    float *xVBOmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                              0, sizeof(float)*Nvertices*2,
                                              GL_MAP_WRITE_BIT|
                                              GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<Nvertices;i++){
        float x = ((float)i/(Nvertices-1));
        xVBOmap[2*i] = x;
        xVBOmap[2*i+1] = x;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindBuffer(GL_ARRAY_BUFFER, yVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Nvertices*2,
                 NULL, GL_DYNAMIC_DRAW);

    float *yVBOmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                              0, sizeof(float)*Nvertices*2,
                                              GL_MAP_WRITE_BIT|
                                              GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<Nvertices;i++){
        yVBOmap[2*i] = 0.0f;
        yVBOmap[2*i+1] = -180.0f;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glBindBuffer(GL_ARRAY_BUFFER, vVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Nvertices*2,
                 NULL, GL_DYNAMIC_DRAW);

    float *vVBOmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                              0, sizeof(float)*Nvertices*2,
                                              GL_MAP_WRITE_BIT|
                                              GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<Nvertices;i++){
        vVBOmap[2*i] = 1.0f;
        vVBOmap[2*i+1] = 0.0f;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

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

GraphFill::~GraphFill(void)
{
    ProgramDestroy();

    glDeleteBuffers(1, &xVBO);
    glDeleteBuffers(1, &yVBO);
    glDeleteBuffers(1, &vVBO);
    glDeleteVertexArrays(1, &VAO);
}

void GraphFill::SetColor(glm::vec4 &color){
    GraphFill::color = color;
}

void GraphFill::SetLimits(float ytop, float ybottom)
{
    GraphFill::ytop = ytop;
    GraphFill::ybottom = ybottom;
}
    
void GraphFill::SetViewWidth(float width)
{
    view_width = width;
}

void GraphFill::SetX(float *x, int N)
{
    glBindBuffer(GL_ARRAY_BUFFER, xVBO);

    float *xVBOmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                              0, sizeof(float)*N,
                                              GL_MAP_WRITE_BIT|
                                              GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<N;i++){
        xVBOmap[2*i] = x[i];
        xVBOmap[2*i+1] = x[i];
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}


void GraphFill::Draw(float *y0, int N)
{
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float thickness = (ytop - ybottom)*0.5f;

    glBindBuffer(GL_ARRAY_BUFFER, yVBO);

    float *yVBOmap = (float *)glMapBufferRange(
        GL_ARRAY_BUFFER,
        0, sizeof(float) * N*2,
        GL_MAP_WRITE_BIT);

    for(int i=0;i<N;i++){
        yVBOmap[i*2] = y0[i];
        yVBOmap[i*2+1] = y0[i]-thickness;
    }

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

    glUniform4fv(colorLocation, 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, N*2);

    glBindVertexArray(0);
    glUseProgram(0);
}



