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

#include "Font.h"
#include "Shader.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include <stdarg.h>
#include <string.h>
#include <cstddef>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

FreeTypeFont::FreeTypeFont( void ){
    loaded = false;
    quad_initialized = false;
}

FreeTypeFont::~FreeTypeFont( void ){
	Free();
    DeleteQuad();
}

void FreeTypeFont::Free( void )
{
    if(!loaded)return;
    for(int c=1;c<128;c++){
        if(characters[c].texture){
            glDeleteTextures(1, &characters[c].texture);
        }
    }
    loaded = false;
}

struct Attributes
{
    glm::vec2 vertex;
    glm::vec2 texel;
};

#define VERTEX_LOC 0
#define TEXEL_LOC  1

void FreeTypeFont::InitQuad(void)
{
    const char *vShaderSrc =
            "#version 400\n"
            "layout(location = 0) in vec2 a_vertex;\n"
            "layout(location = 1) in vec2 a_tex;\n"
            "uniform mat4 mvp;\n"
            "out vec2 tex;\n"
            "void main(void)\n"
            "{\n"
            "   gl_Position = mvp*vec4(a_vertex.xy,0.0,1.0);\n"
            "   tex = a_tex;\n"
            "}\n";

    const char *fShaderSrc =
            "#version 400\n"
            "in vec2 tex;\n"
            "layout(location =0) out vec4 outColor;\n"
            "uniform sampler2D s_texture;\n"
            "void main(void)\n"
            "{\n"
            "   outColor = texture(s_texture, tex);\n"
            "}\n";

    program = LoadProgram(vShaderSrc, fShaderSrc);
    if(!program){
        return;
    }

    mvp_loc = glGetUniformLocation(program, "mvp");
    s_texture_loc = glGetUniformLocation(program, "s_texture");

    Attributes attributes[4] =
    {
        {glm::vec2(0.0,0.0),glm::vec2(0.0,1.0)},
        {glm::vec2(1.0,0.0),glm::vec2(1.0,1.0)},
        {glm::vec2(0.0,1.0),glm::vec2(0.0,0.0)},
        {glm::vec2(1.0,1.0),glm::vec2(1.0,0.0)}
    };

    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(attributes),
                 attributes, GL_STATIC_DRAW);
    glVertexAttribPointer(VERTEX_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Attributes), (void*)offsetof(Attributes,vertex));
    glVertexAttribPointer(TEXEL_LOC, 2, GL_FLOAT, GL_FALSE, sizeof(Attributes), (void*)offsetof(Attributes,texel));
    glEnableVertexAttribArray(VERTEX_LOC);
    glEnableVertexAttribArray(TEXEL_LOC);

    glBindVertexArray(0);
    quad_initialized = true;
}

void FreeTypeFont::DeleteQuad(void)
{
    if(!quad_initialized)return;
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    quad_initialized = false;
}

#define TEXT_BUF_LENGTH 64

void FreeTypeFont::Printf( double x, double y, const char *format, ... )
{
    if(!loaded || !quad_initialized)return;
    char text[TEXT_BUF_LENGTH];

	va_list ap;
	va_start( ap, format );
	vsnprintf( text, TEXT_BUF_LENGTH, format, ap );
	va_end( ap );

	GLint	viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
    float left = 0.0;
    float right = viewport[2];
    float top = viewport[3];
    float bottom = 0.0;
    float nearVal = 1.0;
    float farVal = -1.0;
    glm::mat4 Mprojection = glm::ortho(left, right, bottom, top, nearVal, farVal);

    glm::vec3 vBaseLine(x, y-descender, 0.0);

    glUseProgram(program);
    glBindVertexArray(vao);

    glUniform1i(s_texture_loc, 0);

    glActiveTexture(GL_TEXTURE0);

    for(char *c=text; *c; c++){
        if(characters[*c].texture){
            glBindTexture(GL_TEXTURE_2D, characters[*c].texture);
            glm::mat4 Mscale = glm::scale(characters[*c].scale);
            glm::mat4 Mtrans = glm::translate(vBaseLine + characters[*c].v0);
            glm::mat4 mvp = Mprojection*Mtrans*Mscale;
            glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        vBaseLine += glm::vec3(characters[*c].advance, 0.0, 0.0);
    }

    glUseProgram(0);
    glBindVertexArray(0);
}

float FreeTypeFont::PrintfAdvance(const char *format, ... )
{
    if(!loaded)return 0.0;
    char text[TEXT_BUF_LENGTH];
    va_list ap;
    va_start( ap, format );
    vsnprintf( text, TEXT_BUF_LENGTH, format, ap );
    va_end( ap );

    float advance = 0.0;

    for(char *c=text; *c; c++){
        advance += characters[*c].advance;
    }
    return advance;
}

// Each time the renderer calls us back we just push another span entry on
// our list.

static void
RasterCallback(const int y,
               const int count,
               const FT_Span * const spans,
               void * const user) 
{
  Spans *sptr = (Spans *)user;
  for (int i = 0; i < count; ++i) 
    sptr->push_back(Span(spans[i].x, y, spans[i].len, spans[i].coverage));
}


// Set up the raster parameters and render the outline.

static void
RenderSpans(FT_Library &library,
            FT_Outline * const outline,
            Spans *spans) 
{
  FT_Raster_Params params;
  memset(&params, 0, sizeof(params));
  params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
  params.gray_spans = RasterCallback;
  params.user = spans;

  FT_Outline_Render(library, outline, &params);
}

void FreeTypeFont::LoadCharacter(
         FT_Library library,
         FT_Face face,
         unsigned char charcode,
         glm::vec4 &fontCol,
         glm::vec4 &outlineCol,
         float outlineWidth )
{
// 	printf("make_dlist_outline: charcode=%hhd\n",charcode);
	// Load the glyph we are looking for.
	FT_UInt gindex = FT_Get_Char_Index(face, charcode);
	if (FT_Load_Glyph(face, gindex, FT_LOAD_NO_BITMAP) == 0)
	{
		// Need an outline for this to work.
		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			// Render the basic glyph to a span list.
			Spans spans;
			RenderSpans(library, &face->glyph->outline, &spans);

			// Next we need the spans for the outline.
			Spans outlineSpans;

			// Set up a stroker.
			FT_Stroker stroker;
			FT_Stroker_New(library, &stroker);
			FT_Stroker_Set(stroker,
											(int)(outlineWidth * 64),
											FT_STROKER_LINECAP_ROUND,
											FT_STROKER_LINEJOIN_ROUND,
											0);

			FT_Glyph glyph;
			if (FT_Get_Glyph(face->glyph, &glyph) == 0)
			{
				FT_Glyph_StrokeBorder(&glyph, stroker, 0, 1);
				// Again, this needs to be an outline to work.
				if (glyph->format == FT_GLYPH_FORMAT_OUTLINE)
				{
					// Render the outline spans to the span list
					FT_Outline *o =
						&reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
					RenderSpans(library, o, &outlineSpans);
				}

				// Clean up afterwards.
				FT_Stroker_Done(stroker);
				FT_Done_Glyph(glyph);

				// Now we need to put it all together.
				if (!spans.empty())
				{
					// Figure out what the bounding rect is for both the span lists.
					Rect rect(spans.front().x,
										spans.front().y,
										spans.front().x,
										spans.front().y);
					for (Spans::iterator s = spans.begin();
								s != spans.end(); ++s)
					{
						rect.Include(Vec2(s->x, s->y));
						rect.Include(Vec2(s->x + s->width - 1, s->y));
					}
					for (Spans::iterator s = outlineSpans.begin();
								s != outlineSpans.end(); ++s)
					{
						rect.Include(Vec2(s->x, s->y));
						rect.Include(Vec2(s->x + s->width - 1, s->y));
					}

					// This is unused in this test but you would need this to draw
					// more than one glyph.
// 					float bearingX = (float)face->glyph->metrics.horiBearingX/64;
// 					float bearingY = (float)face->glyph->metrics.horiBearingY/64;
// 					float advance = (float)face->glyph->advance.x/64;
// 					printf("bearingX:%f bearingY:%f advance:%f\n",bearingX,bearingY,advance);
// 					printf("rect: xmin:%f, xmax:%f, ymin:%f ymax:%f\n",
// 								 rect.xmin, rect.xmax, rect.ymin, rect.ymax );

					// Get some metrics of our image.
					int imgWidth = rect.Width(),
							imgHeight = rect.Height(),
							imgSize = imgWidth * imgHeight;

					// Allocate data for our image and clear it out to transparent.
					Pixel32 *pxl = new Pixel32[imgSize];
					memset(pxl, 0, sizeof(Pixel32) * imgSize);

					// Loop over the outline spans and just draw them into the
					// image.
					for (Spans::iterator s = outlineSpans.begin();
								s != outlineSpans.end(); ++s)
						for (int w = 0; w < s->width; ++w)
							pxl[(int)((imgHeight - 1 - (s->y - rect.ymin)) * imgWidth
												+ s->x - rect.xmin + w)] =
                                Pixel32(outlineCol.r*255, outlineCol.g*255, outlineCol.b*255,
												s->coverage);

					// Then loop over the regular glyph spans and blend them into
					// the image.
					for (Spans::iterator s = spans.begin();
								s != spans.end(); ++s)
						for (int w = 0; w < s->width; ++w)
						{
							Pixel32 &dst =
								pxl[(int)((imgHeight - 1 - (s->y - rect.ymin)) * imgWidth
										+ s->x - rect.xmin + w)];
                            Pixel32 src = Pixel32(fontCol.r*255, fontCol.g*255, fontCol.b*255,
																		s->coverage);
							uint8 one_minus_src_alpha = 255 - src.a;
							dst.r = (int)( (dst.r*one_minus_src_alpha + src.r*src.a)/255.0f );
							dst.g = (int)( (dst.g*one_minus_src_alpha + src.g*src.a)/255.0f );
							dst.b = (int)( (dst.b*one_minus_src_alpha + src.b*src.a)/255.0f );
                            dst.a = MAX( dst.a, src.a );
// 							dst.r = (int)(dst.r + ((src.r - dst.r) * src.a) / 255.0f);
// 							dst.g = (int)(dst.g + ((src.g - dst.g) * src.a) / 255.0f);
// 							dst.b = (int)(dst.b + ((src.b - dst.b) * src.a) / 255.0f);
// 							dst.a = MIN(255, dst.a + src.a);
						}

					// write the image to a texture
                    glGenTextures(1, &characters[charcode].texture);
                    glActiveTexture( GL_TEXTURE0 );
                    glBindTexture( GL_TEXTURE_2D, characters[charcode].texture );
//					glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA,
//												imgWidth, imgHeight, 0,
//												GL_RGBA, GL_UNSIGNED_BYTE, pxl );
                    glTexStorage2D( GL_TEXTURE_2D, 1,
                                    GL_RGBA8, imgWidth, imgHeight);
                    glTexSubImage2D( GL_TEXTURE_2D, 0,
                                     0, 0,
                                     imgWidth, imgHeight,
                                     GL_RGBA, GL_UNSIGNED_BYTE, pxl );
                    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

                    characters[charcode].scale = glm::vec3(imgWidth,imgHeight,1.0);
                    characters[charcode].v0 = glm::vec3(rect.xmin, rect.ymin, 0.0);
                    characters[charcode].advance = (float)face->glyph->advance.x / 64;

                    descender = MIN(descender,rect.ymin);
					delete [] pxl;
				}else{
                    characters[charcode].texture = 0;
                    characters[charcode].advance = (float)face->glyph->advance.x / 64;
				}
				
			}
		}
	}
}

void FreeTypeFont::LoadOutline( const FT_Open_Args *args, unsigned int fontsize,
                                glm::vec4 &fontColor, glm::vec4 &outlineColor,
                                float outlineWidth)
{
    if(loaded)Free();
    if(!quad_initialized)InitQuad();
	FT_Library ftlibrary;
	FT_Face    ftface;

	FT_Error error = FT_Init_FreeType( &ftlibrary );
	if(error){
		printf("FT_Init_FreeType:0x%X\n",error);
		return;
	}

	error = FT_Open_Face( ftlibrary, args, 0, &ftface );
	if( error ){
		printf("FT_New_Face:0x%X\n",error);
		FT_Done_FreeType( ftlibrary );
		return;
	}
	
	if(!FT_IS_SCALABLE( ftface ) ){
		printf("Font is not scalable. Not using it.\n");
		FT_Done_Face( ftface );
		FT_Done_FreeType( ftlibrary );
		return;
	}

	error = FT_Set_Pixel_Sizes( ftface, fontsize, fontsize );
	if( error ){
		printf("FT_Set_Pixel_Sizes:0x%X\n",error);
		FT_Done_Face( ftface );
		FT_Done_FreeType( ftlibrary );
		return;
	}

    descender = 0.0;

	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    for( unsigned char c=1;c<128;c++)
        LoadCharacter( ftlibrary, ftface, c,
                       fontColor, outlineColor, outlineWidth );
	
	FT_Done_Face( ftface );
	FT_Done_FreeType( ftlibrary );

    loaded = true;
}



