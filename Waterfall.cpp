/*
  ==============================================================================

    Waterfall.cpp
    Created: 19 Jan 2024 10:11:06am
    Author:  tim

  ==============================================================================
*/

#include "Waterfall.h"
#include "Shader.h"
#include <math.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <iostream>

#define X_VERTEX_LOC 0
#define Y_VERTEX_LOC 1
#define TEXEL_LOC    2

void Waterfall::InitQuads(void)
{
        const char *vShaderSrc =
        "#version 460\n"
        "layout(location = 0) in float a_vertex_x;\n"
        "layout(location = 1) in float a_vertex_y;\n"
        "layout(location = 2) in vec2 a_tex;\n"
        "uniform mat4 mvp;\n"
        "out vec2 tex;\n"
        "void main(void)\n"
        "{\n"
        "   gl_Position = mvp*vec4(a_vertex_x,a_vertex_y,0.0,1.0);\n"
        "   tex = a_tex;\n"
        "}\n";

    const char *fShaderSrc =
        "#version 460\n"
        "in vec2 tex;\n"
        "layout(location =0) out vec4 outColor;\n"
        "uniform sampler2D s_texture;\n"
        "uniform vec4 color_l;\n"
        "uniform vec4 color_r;\n"
        "void main(void)\n"
        "{\n"
        "   vec4 t = texture(s_texture, tex);\n"
        "   outColor = vec4((color_l.rgb*t.r + color_r.rgb*t.g), 1.0);\n"
        "}\n";

    program = LoadProgram(vShaderSrc, fShaderSrc);
    if(!program){
        return;
    }

    mvp_loc = glGetUniformLocation(program, "mvp");
    s_texture_loc = glGetUniformLocation(program, "s_texture");
    color_l_loc = glGetUniformLocation(program, "color_l");
    color_r_loc = glGetUniformLocation(program, "color_r");

    Attributes attributes1[4] =
        {
            {glm::vec2( 0.0, 0.0),glm::vec2(0.0,1.0)},
            {glm::vec2( 1.0, 0.0),glm::vec2(1.0,1.0)},
            {glm::vec2( 0.0, 1.0),glm::vec2(0.0,0.0)},
            {glm::vec2( 1.0, 1.0),glm::vec2(1.0,0.0)}
        };

    Attributes attributes2[4] =
        {
            {glm::vec2( 0.0,-1.0),glm::vec2(0.0,1.0)},
            {glm::vec2( 1.0,-1.0),glm::vec2(1.0,1.0)},
            {glm::vec2( 0.0, 0.0),glm::vec2(0.0,0.0)},
            {glm::vec2( 1.0, 0.0),glm::vec2(1.0,0.0)}
        };


    glGenBuffers(1, &x_vbo);
    glGenBuffers(1, &tex_vbo);
    glGenBuffers(2, y_vbos);
    glGenVertexArrays(2, vaos);

    InitializeBuffers();
    InitializeFrequency();
    
    //
    // the vertex array object for the upper quad
    //
    glBindVertexArray(vaos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, x_vbo);
    glVertexAttribPointer(X_VERTEX_LOC, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(X_VERTEX_LOC);
    
    glBindBuffer(GL_ARRAY_BUFFER, y_vbos[0]);
    glVertexAttribPointer(Y_VERTEX_LOC, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(Y_VERTEX_LOC);
    
    glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
    glVertexAttribPointer(TEXEL_LOC, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(TEXEL_LOC);
    
    //
    // the vertex array object for the lower quad
    //
    glBindVertexArray(vaos[1]);
    glBindBuffer(GL_ARRAY_BUFFER, x_vbo);
    glVertexAttribPointer(X_VERTEX_LOC, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(X_VERTEX_LOC);
    
    glBindBuffer(GL_ARRAY_BUFFER, y_vbos[1]);
    glVertexAttribPointer(Y_VERTEX_LOC, 1, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(Y_VERTEX_LOC);
    
    glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
    glVertexAttribPointer(TEXEL_LOC, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(TEXEL_LOC);
    
    glBindVertexArray(0);

    
    GLint tsize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tsize);
    if( Npoints > tsize ){
        std::cout << "Maximum texture size exceeded." << std::endl;
        return;
    }
    
    glGenTextures(2, textures);
    glActiveTexture(GL_TEXTURE0);
    for(int t=0;t<2;t++){
        glBindTexture(GL_TEXTURE_2D, textures[t]);
        glTexStorage2D(GL_TEXTURE_2D, 1,
                       GL_RG8, Npoints, Nlines);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_AUTO_GENERATE_MIPMAP, GL_FALSE );
    }

    quadsInitialized = true;

}

void Waterfall::DeleteQuads(void)
{
    if(!quadsInitialized) return;
    glDeleteProgram(program);
    glDeleteBuffers(2, y_vbos);
    glDeleteBuffers(1, &x_vbo);
    glDeleteBuffers(1, &tex_vbo);
    glDeleteVertexArrays(2, vaos);
    glDeleteTextures(2, textures);
    quadsInitialized = false;
}

void Waterfall::InitializeBuffers(void)
{
    //
    // y_vbo[0]; the top quad
    //
    glBindBuffer(GL_ARRAY_BUFFER, y_vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Npoints*2,
                 NULL, GL_STATIC_DRAW);
    float *ymap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                           0, sizeof(float)*Npoints*2,
                                           GL_MAP_WRITE_BIT|
                                           GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<Npoints*2;i+=2){
        ymap[i] = 1.0f;
        ymap[i+1] = 0.0f;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
 
    //
    // y_vbos[1]; the bottom quad
    //
    glBindBuffer(GL_ARRAY_BUFFER, y_vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Npoints*2,
                 NULL, GL_STATIC_DRAW);
    ymap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                           0, sizeof(float)*Npoints*2,
                                           GL_MAP_WRITE_BIT|
                                           GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0;i<Npoints*2;i+=2){
        ymap[i] = 0.0f;
        ymap[i+1] = -1.0f;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
 
    //
    // tex_vbo
    //
    glBindBuffer(GL_ARRAY_BUFFER, tex_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2)*Npoints*2,
                 NULL, GL_STATIC_DRAW);
    glm::vec2* tex_map = (glm::vec2*)glMapBufferRange(GL_ARRAY_BUFFER,
                                           0, sizeof(glm::vec2)*Npoints*2,
                                           GL_MAP_WRITE_BIT|
                                           GL_MAP_INVALIDATE_BUFFER_BIT);
    for(int i=0,f=0;i<Npoints*2;i+=2,f++){
        float alpha = (float)f/(Npoints-1);
        tex_map[i].x = alpha;
        tex_map[i].y = 0.0f;
        tex_map[i+1].x = alpha;
        tex_map[i+1].y = 1.0f;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}

void Waterfall::InitializeFrequency(bool log)
{
    glBindBuffer(GL_ARRAY_BUFFER, x_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*Npoints*2,
                 NULL, GL_DYNAMIC_DRAW);
    float *xmap = (float*)glMapBufferRange(GL_ARRAY_BUFFER,
                                           0, sizeof(float)*Npoints*2,
                                           GL_MAP_WRITE_BIT|
                                           GL_MAP_INVALIDATE_BUFFER_BIT);
                                           
    if(!log){
        for(int i=0,f=0;i<Npoints*2;i+=2,f++){
            float alpha = (float)f/(Npoints-1);
            xmap[i] = alpha;
            xmap[i+1] = alpha;
        }
    }else{
        float alpha2 = logf(2.0f)/logf((float)Npoints);
        float beta = alpha2/(1.0f + alpha2);
        float one_m_beta = 1.0f - beta;

        xmap[0] = 0.0f;
        xmap[1] = 0.0f;
        
        for(int i=2,n=1;i<Npoints*2;i+=2,n++){
            float alpha = logf((float)n) / logf((float)Npoints);
            float x = beta + alpha*one_m_beta;
            xmap[i] = x;
            xmap[i+1] = x;
        }
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
}


Waterfall::Waterfall(int Npoints, int Nlines) :
    Npoints(Npoints),
    Nlines(Nlines)
{
    InitQuads();
    if(!quadsInitialized) return;
    line = Nlines;
    texture_phase = true;
    current_tex = textures[0];
    trailing_tex = textures[1];
    view_width = 1.0;
    view_height = 1.0;
    dB_min = -180.0;
    dB_max = 0.0;
    pixels.reset(new wPixel[Npoints]);
}

Waterfall::~Waterfall()
{
    DeleteQuads();
}

void Waterfall::SetViewWidth(float width)
{
    view_width = width;
}

void Waterfall::SetViewHeight(float height)
{
    view_height = height;
}

void Waterfall::SetdBLimits(float dB_min, float dB_max)
{
    Waterfall::dB_min = dB_min;
    Waterfall::dB_max = dB_max;
}

unsigned char Waterfall::dB2intensity(float dB_x)
{
    float intensity;
    if(dB_x >= dB_max){
        intensity = 1.0f;
    }else if(dB_x <= dB_min){
        intensity = 0.0f;
    }else{
        intensity = (dB_x - dB_min)/(dB_max - dB_min);
    }
    intensity*=intensity;
    unsigned char c = intensity*255.0f;
    return c;
}
    

void Waterfall::InsertLine(float *data_l, float *data_r)
{
    if(!quadsInitialized) return;
    if(line==Nlines){
        line = 0;
        if(texture_phase){
            texture_phase = false;
            current_tex = textures[1];
            trailing_tex = textures[0];
        }else{
            texture_phase = true;
            current_tex = textures[0];
            trailing_tex = textures[1];
        }
    }
    for(int i=0;i<Npoints;i++){
        pixels[i].r = dB2intensity(data_l[i]);
        pixels[i].g = dB2intensity(data_r[i]);
    }
    glBindTexture(GL_TEXTURE_2D, current_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, Nlines-line-1, Npoints, 1, GL_RG, GL_UNSIGNED_BYTE, pixels.get());
    line++;
}
        

void Waterfall::Render(glm::vec4 &color_l, glm::vec4 &color_r)
{
    if(!quadsInitialized) return;
    float top = 0.0;
    float bottom = -view_height;
    float left = 0.0;
    float right = view_width;
    
    glm::mat4 M_proj = glm::ortho(left, right, bottom, top);
    glm::vec3 v_trans(0.0f, -(float)line/Nlines, 0.0f);
    glm::mat4 M_trans = glm::translate(v_trans);
    glm::mat4 M_mvp = M_proj*M_trans;
    glUseProgram(program);
    glUniform1i(s_texture_loc, 0);
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(M_mvp));
    glUniform4fv(color_l_loc, 1, glm::value_ptr(color_l));
    glUniform4fv(color_r_loc, 1, glm::value_ptr(color_r));
 
    glActiveTexture(GL_TEXTURE0);
    
    glBindVertexArray(vaos[0]);
    glBindTexture(GL_TEXTURE_2D, current_tex);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, Npoints*2);
    
    glBindVertexArray(vaos[1]);
    glBindTexture(GL_TEXTURE_2D, trailing_tex);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, Npoints*2);
    
    glUseProgram(0);
    glBindVertexArray(0);
}
