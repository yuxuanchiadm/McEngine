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

	void bind(unsigned int textureUnit = 0);
	void unbind();

	void setFilterMode(Graphics::FILTER_MODE filterMode);
	void setWrapMode(Graphics::WRAP_MODE wrapMode);

private:
	void init();
	void initAsync();
	void destroy();

	unsigned int m_GLTexture;
	unsigned int m_iTextureUnitBackup;
};

#endif
