//================ Copyright (c) 2016, PG, All rights reserved. =================//
//
// Purpose:		OpenGL implementation of Image
//
// $NoKeywords: $glimg
//===============================================================================//

#include "OpenGLImage.h"

#include "ResourceManager.h"
#include "Environment.h"
#include "Engine.h"
#include "File.h"

#include "OpenGLHeaders.h"

OpenGLImage::OpenGLImage(UString filepath, bool mipmapped) : Image(filepath, mipmapped)
{
	m_GLTexture = 0;
	m_iTextureUnitBackup = 0;
}

OpenGLImage::OpenGLImage(int width, int height, bool clampToEdge) : Image(width, height, clampToEdge)
{
	m_GLTexture = 0;
	m_iTextureUnitBackup = 0;
}

void OpenGLImage::init()
{
	if (m_GLTexture != 0 || !m_bAsyncReady) return; // only load if we are not already loaded

	if (m_rawImage.size() < m_iWidth*m_iHeight*m_iNumChannels) // sanity check
	{
		engine->showMessageError("Image Error", UString::format("Loaded image has only %i/%i bytes in file %s", m_rawImage.size(), m_iWidth*m_iHeight*m_iNumChannels, m_sFilePath.toUtf8()));
		return;
	}

	if (m_iNumChannels != 4 && m_iNumChannels != 3 && m_iNumChannels != 1) // another sanity check
	{
		engine->showMessageError("Image Error", UString::format("Unsupported number of color channels (%i) in file %s", m_iNumChannels, m_sFilePath.toUtf8()));
		return;
	}

	// TEMP: clear gl error state
	glGetError();

	// create texture and bind
	glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &m_GLTexture);
	glBindTexture(GL_TEXTURE_2D, m_GLTexture);

	// set texture filtering mode (mipmapping is disabled by default)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_bMipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLfloat maxAnisotropy;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);

	// texture wrapping, defaults to clamp
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// TODO: HACKHACK: this if block is only here for backwards compatibility with CGProject and (maybe) MetroidModelViewer (needs check)
	if (m_bCreatedImage) // for lightmaps, which are NPOT
	{
		if (m_bClampToEdge)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	if (m_type == Image::TYPE::TYPE_JPG) // HACKHACK: wat
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//debugLog("going to teximage (mipmapped = %i, alphachannel = %i, width = %i, height = %i, buffer size = %i)\n", (int)m_bMipmapped, (int)m_bHasAlphaChannel, m_iWidth, m_iHeight, m_rawImage.size());

	// m_iNumChannels == 4 ? GL_RGBA : (m_iNumChannels == 3 ? GL_RGB : (m_iNumChannels == 1 ? GL_LUMINANCE : GL_RGBA))
	// set pixel format and load pixels (defaults to no mipmapping)
	if (m_bMipmapped) // TODO: the 4 seems to be wrong? defaults to wrong LOD sometimes... // TODO: gluBuild2DMipmaps() is more or less deprecated, refactor this
		gluBuild2DMipmaps(GL_TEXTURE_2D, 4, m_iWidth, m_iHeight, m_iNumChannels == 4 ? GL_RGBA : (m_iNumChannels == 3 ? GL_RGB : (m_iNumChannels == 1 ? GL_LUMINANCE : GL_RGBA)), GL_UNSIGNED_BYTE, &m_rawImage[0]);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, m_iNumChannels == 4 ? GL_RGBA : (m_iNumChannels == 3 ? GL_RGB : (m_iNumChannels == 1 ? GL_LUMINANCE : GL_RGBA)), m_iWidth, m_iHeight, 0, m_iNumChannels == 4 ? GL_RGBA : (m_iNumChannels == 3 ? GL_RGB : (m_iNumChannels == 1 ? GL_LUMINANCE : GL_RGBA)), GL_UNSIGNED_BYTE, &m_rawImage[0]);

	// free memory
	m_rawImage = std::vector<unsigned char>();

	// check for errors
	int GLerror = glGetError();
	if (GLerror != 0)
	{
		m_GLTexture = 0;
		debugLog("OpenGL Image Error: %i on file %s!\n", GLerror, m_sFilePath.toUtf8());
		engine->showMessageError("Image Error", UString::format("OpenGL Image error %i on file %s", GLerror, m_sFilePath.toUtf8()));
	}
	else
		m_bReady = true;
}

void OpenGLImage::initAsync()
{
	if (m_GLTexture != 0) return; // only load if we are not already loaded

	if (!m_bCreatedImage)
	{
		printf("Resource Manager: Loading %s\n", m_sFilePath.toUtf8());
		m_bAsyncReady = loadRawImage();
	}
}

void OpenGLImage::destroy()
{
	if (m_GLTexture != 0)
		glDeleteTextures(1, &m_GLTexture);
	m_GLTexture = 0;

	m_rawImage = std::vector<unsigned char>();
}

void OpenGLImage::bind(unsigned int textureUnit)
{
	if (!m_bReady) return;

	m_iTextureUnitBackup = textureUnit;

	// switch texture units before enabling+binding
	glActiveTexture(GL_TEXTURE0 + m_iTextureUnitBackup);

	// set texture
	glBindTexture(GL_TEXTURE_2D, m_GLTexture);

	// needed for legacy support (OpenGLLegacyInterface)
	glEnable(GL_TEXTURE_2D);
}

void OpenGLImage::unbind()
{
	if (!m_bReady) return;

	// restore texture unit (just in case) and set to no texture
	glActiveTexture(GL_TEXTURE0 + m_iTextureUnitBackup);
	glBindTexture(GL_TEXTURE_2D, 0);

	// restore default texture unit
	if (m_iTextureUnitBackup != 0)
		glActiveTexture(GL_TEXTURE0);
}

void OpenGLImage::setFilterMode(Graphics::FILTER_MODE filterMode)
{
	if (!m_bReady)
		return;

	bind();
	{
		switch (filterMode)
		{
		case Graphics::FILTER_MODE::FILTER_MODE_NONE:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			break;
		case Graphics::FILTER_MODE::FILTER_MODE_LINEAR:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		case Graphics::FILTER_MODE::FILTER_MODE_MIPMAP:
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			break;
		}
	}
	unbind();
}

void OpenGLImage::setWrapMode(Graphics::WRAP_MODE wrapMode)
{
	if (!m_bReady)
		return;

	bind();
	{
		switch (wrapMode)
		{
		case Graphics::WRAP_MODE::WRAP_MODE_CLAMP: // NOTE: there is also GL_CLAMP, which works a bit differently concerning the border color
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			break;
		case Graphics::WRAP_MODE::WRAP_MODE_REPEAT:
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
		}
	}
	unbind();
}
