//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of Image
//
// $NoKeywords: $glimg
//===============================================================================//

#ifndef OPENGLIMAGE_H
#define OPENGLIMAGE_H

#include "Image.h"

class OpenGLImage : public Image
{
public:
	OpenGLImage(UString filepath, bool mipmapped = false);
	OpenGLImage(int width, int height, bool clampToEdge = true);
	virtual ~OpenGLImage() {destroy();}

	virtual void bind(unsigned int textureUnit = 0);
	virtual void unbind();

	virtual void setFilterMode(Graphics::FILTER_MODE filterMode);
	virtual void setWrapMode(Graphics::WRAP_MODE wrapMode);

private:
	virtual void init();
	virtual void initAsync();
	virtual void destroy();

	unsigned int m_GLTexture;
	unsigned int m_iTextureUnitBackup;
};

#endif
