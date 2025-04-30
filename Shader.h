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

    Shader.h
    Created: 18 Jan 2024 8:59:01pm
    Author:  tim

  ==============================================================================
*/

#pragma once

#include <glad/gl.h>

GLuint LoadShader(GLenum type, const char *shaderSrc);
GLuint LoadProgram(const char *vertShaderSrc, const char *fragShaderSrc);


