/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Texture.h"

#include <stdexcept>

#include "Global/GLIncludes.h"


NATRON_NAMESPACE_ENTER;

Texture::Texture(U32 target,
                 int minFilter,
                 int magFilter,
                 int clamp,
                 DataTypeEnum type,
                 int format,
                 int internalFormat,
                 int glType,
                 bool useOpenGL)
    : _texID(0)
    , _target(target)
    , _minFilter(minFilter)
    , _magFilter(magFilter)
    , _clamp(clamp)
    , _internalFormat(internalFormat)
    , _format(format)
    , _glType(glType)
    , _type(type)
    , _useOpenGL(useOpenGL)
{
    if (useOpenGL) {
        GL_GPU::glGenTextures(1, &_texID);
    } else {
        GL_CPU::glGenTextures(1, &_texID);
    }
}

void
Texture::getRecommendedTexParametersForRGBAByteTexture(int* format,
                                                       int* internalFormat,
                                                       int* glType)
{
    *format = GL_BGRA;
    *internalFormat = GL_RGBA8;
    *glType = GL_UNSIGNED_INT_8_8_8_8_REV;
}

void
Texture::getRecommendedTexParametersForRGBAFloatTexture(int* format, int* internalFormat, int* glType)
{
    *format = GL_RGBA;
    *internalFormat = GL_RGBA32F_ARB;
    *glType = GL_FLOAT;
}

template <typename GL>
void ensureTextureHasSizeInternal(const unsigned char* originalRAMBuffer,
                                  int target,
                                  int texID,
                                  int minFilter,
                                  int magFilter,
                                  int clamp,
                                  int internalFormat,
                                  int format,
                                  int glType,
                                  int w,
                                  int h)

{
    GLProtectAttrib<GL> a(GL_ENABLE_BIT);
    GL::glEnable(target);
    GL::glBindTexture (target, texID);

    GL::glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    if (minFilter != GL_NONE) {
        GL::glTexParameteri (target, GL_TEXTURE_MIN_FILTER, minFilter);
    }
    if (magFilter != GL_NONE) {
        GL::glTexParameteri (target, GL_TEXTURE_MAG_FILTER, magFilter);
    }

    if (clamp != GL_NONE) {
        GL::glTexParameteri (target, GL_TEXTURE_WRAP_S, clamp);
        GL::glTexParameteri (target, GL_TEXTURE_WRAP_T, clamp);
    }

    GL::glTexImage2D (target,
                  0,            // level
                  internalFormat, //internalFormat
                  w, h,
                  0,            // border
                  format,      // format
                  glType, // type
                  originalRAMBuffer);           // pixels


    GL::glBindTexture(target, 0);
    glCheckError(GL);
}

bool
Texture::ensureTextureHasSize(const TextureRect& texRect,
                              const unsigned char* originalRAMBuffer)
{
    if (texRect == _textureRect) {
        return false;
    }
    _textureRect = texRect;

    if (_useOpenGL) {
        ensureTextureHasSizeInternal<GL_GPU>(originalRAMBuffer, _target, _texID, _minFilter, _magFilter, _clamp, _internalFormat, _format, _glType, w(), h());
    } else {
        ensureTextureHasSizeInternal<GL_CPU>(originalRAMBuffer, _target, _texID, _minFilter, _magFilter, _clamp, _internalFormat, _format, _glType, w(), h());
    }

    return true;
}

template <typename GL>
void fillOrAllocateTextureInternal(const TextureRect & texRect,
                                   Texture* texture,
                                   const RectI& roi,
                                   bool updateOnlyRoi,
                                   const unsigned char* originalRAMBuffer,
                                   int target,
                                   int texID,
                                   int format,
                                   int glType,
                                   int w,
                                   int h)
{
    //GLuint savedTexture;
    //glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&savedTexture);
    {
        GLProtectAttrib<GL> a(GL_ENABLE_BIT);
        int width = updateOnlyRoi ? roi.width() : w;
        int height = updateOnlyRoi ? roi.height() : h;
        int x1 = updateOnlyRoi ? roi.x1 - texRect.x1 : 0;
        int y1 = updateOnlyRoi ? roi.y1 - texRect.y1 : 0;

        if ( !texture->ensureTextureHasSize(texRect, originalRAMBuffer) ) {
            GL::glEnable(target);
            GL::glBindTexture (target, texID);

            GL::glTexSubImage2D(target,
                            0,              // level
                            x1, y1,               // xoffset, yoffset
                            width, height,
                            format,            // format
                            glType,       // type
                            originalRAMBuffer);

            GL::glBindTexture (target, 0);
            glCheckError(GL);
        }
    } // GLProtectAttrib a(GL_ENABLE_BIT);
}

void
Texture::fillOrAllocateTexture(const TextureRect & texRect,
                               const RectI& roi,
                               bool updateOnlyRoi,
                               const unsigned char* originalRAMBuffer)
{
    if (_useOpenGL) {
        fillOrAllocateTextureInternal<GL_GPU>(texRect, this, roi, updateOnlyRoi, originalRAMBuffer, _target, _texID, _format, _glType, w(), h());
    } else {
        fillOrAllocateTextureInternal<GL_CPU>(texRect, this, roi, updateOnlyRoi, originalRAMBuffer, _target, _texID, _format, _glType, w(), h());
    }
} // fillOrAllocateTexture

Texture::~Texture()
{
    if (_useOpenGL) {
        GL_GPU::glDeleteTextures(1, &_texID);
    } else {
        GL_CPU::glDeleteTextures(1, &_texID);
    }
}

NATRON_NAMESPACE_EXIT;
