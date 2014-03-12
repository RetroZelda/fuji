#pragma once
#if !defined(_MFTEXTURE_INTERNAL_H)
#define _MFTEXTURE_INTERNAL_H

#if MF_RENDERER == MF_DRIVER_D3D9
	#include <d3d9.h>
#elif MF_RENDERER == MF_DRIVER_OPENGL
	#include "../Source/Drivers/OpenGL/MFOpenGL.h"
#endif

#include "MFTexture.h"
#include "MFResource.h"

struct MFTextureSurfaceLevel
{
	int width, height;
	int bitsPerPixel;

	int xBlocks, yBlocks;
	int bitsPerBlock;

	char *pImageData;
	int bufferLength;

	char *pPaletteEntries;
	int paletteBufferLength;

	uint32 res[2];
};

// texture TemplateData
struct MFTextureTemplateData
{
	uint32 magicNumber;
	MFImageFormat imageFormat;
	uint32 reserved;
	int mipLevels;
	uint32 flags;

	// padding
	uint32 res[2];

	MFTextureSurfaceLevel *pSurfaces;
};

// texture structure
struct MFTexture : public MFResource
{
	MFTextureTemplateData *pTemplateData;

#if MF_RENDERER == MF_DRIVER_XBOX
#if defined(XB_XGTEXTURES)
	IDirect3DTexture8 texture;
#endif
	IDirect3DTexture8 *pTexture;
#elif MF_RENDERER == MF_DRIVER_PS2
	unsigned int vramAddr;
#else
	void *pInternalData;
#endif
};

// functions
MFInitStatus MFTexture_InitModule(int moduleId, bool bPerformInitialisation);
void MFTexture_DeinitModule();

void MFTexture_InitModulePlatformSpecific();
void MFTexture_DeinitModulePlatformSpecific();

void MFTexture_CreatePlatformSpecific(MFTexture *pTexture, bool generateMipChain);
void MFTexture_DestroyPlatformSpecific(MFTexture *pTexture);

#if !defined(_FUJI_UTIL)
// a debug menu texture information display object
#include "DebugMenu_Internal.h"

class TextureBrowser : public MenuObject
{
public:
	TextureBrowser();

	virtual void Draw();
	virtual void Update();

	virtual float ListDraw(bool selected, const MFVector &pos, float maxWidth);
	virtual void ListUpdate(bool selected);
	virtual MFVector GetDimensions(float maxWidth);

	int selection;
};
#endif

#endif
