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

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "fontbase.h"

struct Character
{
    GLuint    texture;
    glm::vec3 scale; // scaling factor for the quad
    glm::vec3 v0;    // location of v0 relative to baseline
    float     advance; // amount to advance on the baseline
};

class FreeTypeFont
{
private:
    bool      loaded;
    bool      quad_initialized;
    Character characters[128];
    GLuint    program;
    GLuint    vbo;
    GLuint    vao;
    GLint     mvp_loc;
    GLint     s_texture_loc;
    float     descender;

    void InitQuad(void);
    void DeleteQuad(void);

    void LoadCharacter(
            FT_Library library,
            FT_Face face,
            unsigned char charcode,
            glm::vec4 &fontCol,
            glm::vec4 &outlineCol,
            float outlineWidth );

public:
	FreeTypeFont( void );
    ~FreeTypeFont( );

	void LoadOutline( const FT_Open_Args *args, unsigned int fontsize,
                      glm::vec4 &fontColor, glm::vec4 &outlineColor,
                      float outlineWidth);
    void Free( void );
    void Printf( double x, double y, const char *format, ... );
    float PrintfAdvance(const char *format, ...);
};
