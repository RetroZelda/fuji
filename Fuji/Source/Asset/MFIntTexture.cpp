#include "Fuji_Internal.h"

#define MF_ENABLE_PNG

#if defined(MF_ENABLE_PNG)
	#if defined(MF_LINUX) || defined(MF_OSX)
		#include <png.h>
	#else
		#include "png.h"
		#include "pngstruct.h"
		#include "pnginfo.h"
	#endif
#endif

#include "MFSystem.h"
#include "MFFileSystem.h"
#include "MFHeap.h"
#include "MFString.h"
#include "MFImage.h"
#include "MFTexture_Internal.h"
#include "Util.h"
#include "Asset/MFIntTexture.h"

#if defined(MF_WINDOWS)
//	#define MF_ENABLE_ATI_COMPRESSOR
#endif
#if defined(MF_WINDOWS) || defined(MF_XBOX)
	#define MF_ENABLE_MS_COMPRESSOR
#endif

#if defined(MF_ENABLE_ATI_COMPRESSOR)
	#include <windows.h>
	#include "ATI_Compress/ATI_Compress.h"
	#pragma comment(lib, "ATI_Compress/ATI_Compress.lib")
#endif

/**** Structures ****/

#pragma pack(1)
struct TgaHeader
{
	uint8 idLength;
	uint8 colourMapType;
	uint8 imageType;

	uint16 colourMapStart;
	uint16 colourMapLength;
	uint8 colourMapBits;

	uint16 xStart;
	uint16 yStart;
	uint16 width;
	uint16 height;
	uint8 bpp;
	uint8 flags;
};
#pragma pack ()

// BMP support

enum BMPCompressionType
{
	BMCT_RGB = 0,		/* No compression - straight BGR data */
	BMCT_RLE8 = 1,		/* 8-bit run-length compression */
	BMCT_RLE4 = 2,		/* 4-bit run-length compression */
	BMCT_BITFIELDS = 3	/* RGB bitmap with RGB masks */
};

#pragma pack(1)
struct BMPHeader
{
   unsigned short int type;                 /* Magic identifier            */
   unsigned int size;                       /* File size in bytes          */
   unsigned short int reserved1, reserved2;
   unsigned int offset;                     /* Offset to image data, bytes */
};

struct BMPInfoHeader
{
   unsigned int size;               /* Header size in bytes      */
   int width,height;                /* Width and height of image */
   unsigned short int planes;       /* Number of colour planes   */
   unsigned short int bits;         /* Bits per pixel            */
   unsigned int compression;        /* Compression type          */
   unsigned int imagesize;          /* Image size in bytes       */
   int xresolution,yresolution;     /* Pixels per meter          */
   unsigned int ncolours;           /* Number of colours         */
   unsigned int importantcolours;   /* Important colours         */
};
#pragma pack ()

struct BMPPaletteEntry /**** Colourmap entry structure ****/
{
	unsigned char  rgbBlue;          /* Blue value */
	unsigned char  rgbGreen;         /* Green value */
	unsigned char  rgbRed;           /* Red value */
	unsigned char  rgbReserved;      /* Reserved */
};

// DDS support

struct DDS_PIXELFORMAT {
  uint32 dwSize;
  uint32 dwFlags;
  uint32 dwFourCC;
  uint32 dwRGBBitCount;
  uint32 dwRBitMask;
  uint32 dwGBitMask;
  uint32 dwBBitMask;
  uint32 dwABitMask;
};

typedef struct {
  uint32          dwSize;
  uint32          dwFlags;
  uint32          dwHeight;
  uint32          dwWidth;
  uint32          dwPitchOrLinearSize;
  uint32          dwDepth;
  uint32          dwMipMapCount;
  uint32          dwReserved1[11];
  DDS_PIXELFORMAT ddspf;
  uint32          dwCaps;
  uint32          dwCaps2;
  uint32          dwCaps3;
  uint32          dwCaps4;
  uint32          dwReserved2;
} DDS_HEADER;

typedef enum DXGI_FORMAT { 
  DXGI_FORMAT_UNKNOWN                     = 0,
  DXGI_FORMAT_R32G32B32A32_TYPELESS       = 1,
  DXGI_FORMAT_R32G32B32A32_FLOAT          = 2,
  DXGI_FORMAT_R32G32B32A32_UINT           = 3,
  DXGI_FORMAT_R32G32B32A32_SINT           = 4,
  DXGI_FORMAT_R32G32B32_TYPELESS          = 5,
  DXGI_FORMAT_R32G32B32_FLOAT             = 6,
  DXGI_FORMAT_R32G32B32_UINT              = 7,
  DXGI_FORMAT_R32G32B32_SINT              = 8,
  DXGI_FORMAT_R16G16B16A16_TYPELESS       = 9,
  DXGI_FORMAT_R16G16B16A16_FLOAT          = 10,
  DXGI_FORMAT_R16G16B16A16_UNORM          = 11,
  DXGI_FORMAT_R16G16B16A16_UINT           = 12,
  DXGI_FORMAT_R16G16B16A16_SNORM          = 13,
  DXGI_FORMAT_R16G16B16A16_SINT           = 14,
  DXGI_FORMAT_R32G32_TYPELESS             = 15,
  DXGI_FORMAT_R32G32_FLOAT                = 16,
  DXGI_FORMAT_R32G32_UINT                 = 17,
  DXGI_FORMAT_R32G32_SINT                 = 18,
  DXGI_FORMAT_R32G8X24_TYPELESS           = 19,
  DXGI_FORMAT_D32_FLOAT_S8X24_UINT        = 20,
  DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS    = 21,
  DXGI_FORMAT_X32_TYPELESS_G8X24_UINT     = 22,
  DXGI_FORMAT_R10G10B10A2_TYPELESS        = 23,
  DXGI_FORMAT_R10G10B10A2_UNORM           = 24,
  DXGI_FORMAT_R10G10B10A2_UINT            = 25,
  DXGI_FORMAT_R11G11B10_FLOAT             = 26,
  DXGI_FORMAT_R8G8B8A8_TYPELESS           = 27,
  DXGI_FORMAT_R8G8B8A8_UNORM              = 28,
  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB         = 29,
  DXGI_FORMAT_R8G8B8A8_UINT               = 30,
  DXGI_FORMAT_R8G8B8A8_SNORM              = 31,
  DXGI_FORMAT_R8G8B8A8_SINT               = 32,
  DXGI_FORMAT_R16G16_TYPELESS             = 33,
  DXGI_FORMAT_R16G16_FLOAT                = 34,
  DXGI_FORMAT_R16G16_UNORM                = 35,
  DXGI_FORMAT_R16G16_UINT                 = 36,
  DXGI_FORMAT_R16G16_SNORM                = 37,
  DXGI_FORMAT_R16G16_SINT                 = 38,
  DXGI_FORMAT_R32_TYPELESS                = 39,
  DXGI_FORMAT_D32_FLOAT                   = 40,
  DXGI_FORMAT_R32_FLOAT                   = 41,
  DXGI_FORMAT_R32_UINT                    = 42,
  DXGI_FORMAT_R32_SINT                    = 43,
  DXGI_FORMAT_R24G8_TYPELESS              = 44,
  DXGI_FORMAT_D24_UNORM_S8_UINT           = 45,
  DXGI_FORMAT_R24_UNORM_X8_TYPELESS       = 46,
  DXGI_FORMAT_X24_TYPELESS_G8_UINT        = 47,
  DXGI_FORMAT_R8G8_TYPELESS               = 48,
  DXGI_FORMAT_R8G8_UNORM                  = 49,
  DXGI_FORMAT_R8G8_UINT                   = 50,
  DXGI_FORMAT_R8G8_SNORM                  = 51,
  DXGI_FORMAT_R8G8_SINT                   = 52,
  DXGI_FORMAT_R16_TYPELESS                = 53,
  DXGI_FORMAT_R16_FLOAT                   = 54,
  DXGI_FORMAT_D16_UNORM                   = 55,
  DXGI_FORMAT_R16_UNORM                   = 56,
  DXGI_FORMAT_R16_UINT                    = 57,
  DXGI_FORMAT_R16_SNORM                   = 58,
  DXGI_FORMAT_R16_SINT                    = 59,
  DXGI_FORMAT_R8_TYPELESS                 = 60,
  DXGI_FORMAT_R8_UNORM                    = 61,
  DXGI_FORMAT_R8_UINT                     = 62,
  DXGI_FORMAT_R8_SNORM                    = 63,
  DXGI_FORMAT_R8_SINT                     = 64,
  DXGI_FORMAT_A8_UNORM                    = 65,
  DXGI_FORMAT_R1_UNORM                    = 66,
  DXGI_FORMAT_R9G9B9E5_SHAREDEXP          = 67,
  DXGI_FORMAT_R8G8_B8G8_UNORM             = 68,
  DXGI_FORMAT_G8R8_G8B8_UNORM             = 69,
  DXGI_FORMAT_BC1_TYPELESS                = 70,
  DXGI_FORMAT_BC1_UNORM                   = 71,
  DXGI_FORMAT_BC1_UNORM_SRGB              = 72,
  DXGI_FORMAT_BC2_TYPELESS                = 73,
  DXGI_FORMAT_BC2_UNORM                   = 74,
  DXGI_FORMAT_BC2_UNORM_SRGB              = 75,
  DXGI_FORMAT_BC3_TYPELESS                = 76,
  DXGI_FORMAT_BC3_UNORM                   = 77,
  DXGI_FORMAT_BC3_UNORM_SRGB              = 78,
  DXGI_FORMAT_BC4_TYPELESS                = 79,
  DXGI_FORMAT_BC4_UNORM                   = 80,
  DXGI_FORMAT_BC4_SNORM                   = 81,
  DXGI_FORMAT_BC5_TYPELESS                = 82,
  DXGI_FORMAT_BC5_UNORM                   = 83,
  DXGI_FORMAT_BC5_SNORM                   = 84,
  DXGI_FORMAT_B5G6R5_UNORM                = 85,
  DXGI_FORMAT_B5G5R5A1_UNORM              = 86,
  DXGI_FORMAT_B8G8R8A8_UNORM              = 87,
  DXGI_FORMAT_B8G8R8X8_UNORM              = 88,
  DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM  = 89,
  DXGI_FORMAT_B8G8R8A8_TYPELESS           = 90,
  DXGI_FORMAT_B8G8R8A8_UNORM_SRGB         = 91,
  DXGI_FORMAT_B8G8R8X8_TYPELESS           = 92,
  DXGI_FORMAT_B8G8R8X8_UNORM_SRGB         = 93,
  DXGI_FORMAT_BC6H_TYPELESS               = 94,
  DXGI_FORMAT_BC6H_UF16                   = 95,
  DXGI_FORMAT_BC6H_SF16                   = 96,
  DXGI_FORMAT_BC7_TYPELESS                = 97,
  DXGI_FORMAT_BC7_UNORM                   = 98,
  DXGI_FORMAT_BC7_UNORM_SRGB              = 99,
  DXGI_FORMAT_AYUV                        = 100,
  DXGI_FORMAT_Y410                        = 101,
  DXGI_FORMAT_Y416                        = 102,
  DXGI_FORMAT_NV12                        = 103,
  DXGI_FORMAT_P010                        = 104,
  DXGI_FORMAT_P016                        = 105,
  DXGI_FORMAT_420_OPAQUE                  = 106,
  DXGI_FORMAT_YUY2                        = 107,
  DXGI_FORMAT_Y210                        = 108,
  DXGI_FORMAT_Y216                        = 109,
  DXGI_FORMAT_NV11                        = 110,
  DXGI_FORMAT_AI44                        = 111,
  DXGI_FORMAT_IA44                        = 112,
  DXGI_FORMAT_P8                          = 113,
  DXGI_FORMAT_A8P8                        = 114,
  DXGI_FORMAT_B4G4R4A4_UNORM              = 115,
  DXGI_FORMAT_FORCE_UINT                  = 0xffffffffUL
} DXGI_FORMAT;

typedef enum D3D10_RESOURCE_DIMENSION { 
  D3D10_RESOURCE_DIMENSION_UNKNOWN    = 0,
  D3D10_RESOURCE_DIMENSION_BUFFER     = 1,
  D3D10_RESOURCE_DIMENSION_TEXTURE1D  = 2,
  D3D10_RESOURCE_DIMENSION_TEXTURE2D  = 3,
  D3D10_RESOURCE_DIMENSION_TEXTURE3D  = 4
} D3D10_RESOURCE_DIMENSION;

typedef struct {
  DXGI_FORMAT              dxgiFormat;
  D3D10_RESOURCE_DIMENSION resourceDimension;
  uint32                   miscFlag;
  uint32                   arraySize;
  uint32                   miscFlags2;
} DDS_HEADER_DXT10;

struct DDS_IMAGE
{
	uint32 magic; // == 0x20534444
	DDS_HEADER header;
	union
	{
		char imageData[1];
		struct
		{
			DDS_HEADER_DXT10 dx10Header;
			char imageData10[1];
		};
	};
};

/**** Globals ****/

static const char *gFileExtensions[] =
{
	".tga",
	".bmp",
	".png",
	".dds",
	".jpg",
	".jpeg",
	".webp"
};

static const int gNumFileExtensions = sizeof(gFileExtensions) / sizeof(gFileExtensions[0]);

MFIntTextureFormat gFileTypeMap[gNumFileExtensions] =
{
	MFITF_TGA,
	MFITF_BMP,
	MFITF_PNG,
	MFITF_DDS,
	MFITF_JPEG,
	MFITF_JPEG,
	MFITF_WEBP
};


/**** Functions ****/

#if defined(MF_ENABLE_PNG)
void PNGAPI png_file_read(png_structp png, png_bytep pBuffer, png_size_t bytes)
{
	MFFile_Read((MFFile*)png->io_ptr, pBuffer, (uint32)bytes, false);
}

MFIntTexture* LoadPNG(const void *pMemory, size_t size)
{
	if(png_sig_cmp((uint8*)pMemory, 0, 8))
	{
		MFDebug_Warn(2, "Not a PNG file..");
		return NULL;
	}

	MFFile *pFile = MFFile_CreateMemoryFile(pMemory, size);

	if(!pFile)
		return NULL;

	// skip past the sig
	MFFile_Seek(pFile, 8, MFSeek_Begin);

	int width, height;
	png_byte color_type;
	png_byte bit_depth;

	png_structp png_ptr;
	png_infop info_ptr;
	int number_of_passes;
	png_bytep * row_pointers;

	// initialize stuff
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
//	setjmp(png_jmpbuf(png_ptr));

	png_set_read_fn(png_ptr, pFile, png_file_read);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	width = info_ptr->width;
	height = info_ptr->height;
	color_type = info_ptr->color_type;
	bit_depth = info_ptr->bit_depth;

	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	// read file
//	setjmp(png_jmpbuf(png_ptr));

	row_pointers = (png_bytep*)MFHeap_Alloc((sizeof(png_bytep) + info_ptr->rowbytes)*height);
	for(int y=0; y<height; y++)
		row_pointers[y] = (png_byte*)(row_pointers + height) + info_ptr->rowbytes*y;

	png_read_image(png_ptr, row_pointers);

	MFFile_Close(pFile);

	// allocate internal image structures
	MFIntTexture *pImage = (MFIntTexture*)MFHeap_Alloc(sizeof(MFIntTexture));

	pImage->numSurfaces = 1;
	pImage->pSurfaces = (MFIntTextureSurface*)MFHeap_Alloc(sizeof(MFIntTextureSurface));

	pImage->pSurfaces[0].pData = (MFIntTexturePixel*)MFHeap_Alloc(sizeof(MFIntTexturePixel)*width*height);
	pImage->pSurfaces[0].width = width;
	pImage->pSurfaces[0].height = height;

	MFIntTexturePixel *pPixel = pImage->pSurfaces[0].pData;

	switch(color_type)
	{
		case PNG_COLOR_TYPE_GRAY:
			for(int y=0; y<height; ++y)
			{
				if(bit_depth == 8)
				{
					uint8 *p = row_pointers[y];

					for(int x=0; x<width; ++x)
					{
						pPixel->r = (float)*p * (1.0f/255.0f);
						pPixel->g = (float)*p * (1.0f/255.0f);
						pPixel->b = (float)*p * (1.0f/255.0f);
						pPixel->a = 1.0f;

						++p;
						++pPixel;
					}
				}
				else if(bit_depth == 16)
				{
					uint16 *p = (uint16*)row_pointers[y];

					for(int x=0; x<width; ++x)
					{
						pPixel->r = (float)*p * (1.0f/65535.0f);
						pPixel->g = (float)*p * (1.0f/65535.0f);
						pPixel->b = (float)*p * (1.0f/65535.0f);
						pPixel->a = 1.0f;

						++p;
						++pPixel;
					}
				}
				else
				{
					MFDebug_Assert(false, "Invalid bit depth!");
				}
			}
			break;
		case PNG_COLOR_TYPE_PALETTE:
			break;
		case PNG_COLOR_TYPE_RGB:
			for(int y=0; y<height; ++y)
			{
				if(bit_depth == 8)
				{
					uint8 *p = row_pointers[y];

					for(int x=0; x<width; ++x)
					{
						pPixel->r = (float)p[0] * (1.0f/255.0f);
						pPixel->g = (float)p[1] * (1.0f/255.0f);
						pPixel->b = (float)p[2] * (1.0f/255.0f);
						pPixel->a = 1.0f;

						p += 3;
						++pPixel;
					}
				}
				else if(bit_depth == 16)
				{
					uint16 *p = (uint16*)row_pointers[y];

					for(int x=0; x<width; ++x)
					{
						pPixel->r = (float)p[0] * (1.0f/65535.0f);
						pPixel->g = (float)p[1] * (1.0f/65535.0f);
						pPixel->b = (float)p[2] * (1.0f/65535.0f);
						pPixel->a = 1.0f;

						p += 3;
						++pPixel;
					}
				}
				else
				{
					MFDebug_Assert(false, "Invalid bit depth!");
				}
			}
			break;
		case PNG_COLOR_TYPE_RGB_ALPHA:
			for(int y=0; y<height; ++y)
			{
				if(bit_depth == 8)
				{
					uint8 *p = row_pointers[y];

					for(int x=0; x<width; ++x)
					{
						pPixel->r = (float)p[0] * (1.0f/255.0f);
						pPixel->g = (float)p[1] * (1.0f/255.0f);
						pPixel->b = (float)p[2] * (1.0f/255.0f);
						pPixel->a = (float)p[3] * (1.0f/255.0f);

						p += 4;
						++pPixel;
					}
				}
				else if(bit_depth == 16)
				{
					uint16 *p = (uint16*)row_pointers[y];

					for(int x=0; x<width; ++x)
					{
						pPixel->r = (float)p[0] * (1.0f/65535.0f);
						pPixel->g = (float)p[1] * (1.0f/65535.0f);
						pPixel->b = (float)p[2] * (1.0f/65535.0f);
						pPixel->a = (float)p[3] * (1.0f/65535.0f);

						p += 4;
						++pPixel;
					}
				}
				else
				{
					MFDebug_Assert(false, "Invalid bit depth!");
				}
			}
			break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:
			for(int y=0; y<height; ++y)
			{
				if(bit_depth == 8)
				{
					uint8 *p = row_pointers[y];

					for(int x=0; x<width; ++x)
					{
						pPixel->r = (float)p[0] * (1.0f/255.0f);
						pPixel->g = (float)p[0] * (1.0f/255.0f);
						pPixel->b = (float)p[0] * (1.0f/255.0f);
						pPixel->a = (float)p[1] * (1.0f/255.0f);

						p += 2;
						++pPixel;
					}
				}
				else if(bit_depth == 16)
				{
					uint16 *p = (uint16*)row_pointers[y];

					for(int x=0; x<width; ++x)
					{
						pPixel->r = (float)p[0] * (1.0f/65535.0f);
						pPixel->g = (float)p[0] * (1.0f/65535.0f);
						pPixel->b = (float)p[0] * (1.0f/65535.0f);
						pPixel->a = (float)p[1] * (1.0f/65535.0f);

						p += 2;
						++pPixel;
					}
				}
				else
				{
					MFDebug_Assert(false, "Invalid bit depth!");
				}
			}
			break;
	}

	// free image
	MFHeap_Free(row_pointers);

	return pImage;
}
#endif

MFIntTexture* LoadTGA(const void *pMemory, size_t imageSize)
{
	unsigned char *pTarga = (unsigned char *)pMemory;

	if(imageSize < (sizeof(TgaHeader) + 1))
		return NULL;

	TgaHeader *pHeader = (TgaHeader*)pTarga;
	unsigned char *pImageData = pTarga + sizeof(TgaHeader);

	MFIntTexture *pImage = (MFIntTexture*)MFHeap_Alloc(sizeof(MFIntTexture));

	pImage->numSurfaces = 1;
	pImage->pSurfaces = (MFIntTextureSurface*)MFHeap_Alloc(sizeof(MFIntTextureSurface));

	pImage->pSurfaces[0].pData = (MFIntTexturePixel*)MFHeap_Alloc(sizeof(MFIntTexturePixel)*pHeader->width*pHeader->height);
	pImage->pSurfaces[0].width = pHeader->width;
	pImage->pSurfaces[0].height = pHeader->height;

	MFIntTexturePixel *pPixel = pImage->pSurfaces[0].pData;

	unsigned char *pPosition;
	bool isSavedFlipped = true;

	pPosition = pTarga;
	pPosition += sizeof(TgaHeader);

	if((pHeader->imageType != 1) && (pHeader->imageType != 2) && (pHeader->imageType != 10))
	{
		MFDebug_Warn(2, MFStr("Failed loading image (Unhandled TGA type (%d))\n", pHeader->imageType));
		return NULL;
	}

	if((pHeader->bpp != 24) && (pHeader->bpp != 32) && (pHeader->bpp != 16))
	{
		MFDebug_Warn(2, MFStr("Failed loading image (Invalid colour depth (%d))", pHeader->bpp));
		return NULL;
	}

	if((pHeader->flags & 0xC0))
	{
		MFDebug_Warn(2, "Failed loading image (Interleaved images not supported)");
		return NULL;
	}

	if((pHeader->flags & 0x20) >> 5)
	{
		isSavedFlipped = false;
	}

	if((pPosition + pHeader->idLength + (pHeader->colourMapLength * pHeader->colourMapBits * pHeader->colourMapType)) >= pTarga + imageSize)
	{
		MFDebug_Warn(2, "Failed loading image (Unexpected end of file)");
		return NULL;
	}

	pPosition += pHeader->idLength;

	int bytesPerPixel = pHeader->bpp/8;

	if(pHeader->imageType == 10) // RLE, ick...
	{
		uint32 pixelsRead = 0;

		while(pixelsRead < (uint32)(pHeader->width * pHeader->height))
		{
			if(pPosition >= pTarga + imageSize)
			{
				MFDebug_Warn(2, "Failed loading image (Unexpected end of file)");
				return NULL;
			}

			if(*pPosition & 0x80) // Run length packet
			{
				uint8 length = ((*pPosition) & 0x7F) + 1;

				pPosition += 1;

				if((pPosition + bytesPerPixel) > pTarga + imageSize)
				{
					MFDebug_Warn(2, "Failed loading image (Unexpected end of file)");
					return NULL;
				}

				if((pixelsRead + length) > (uint32)(pHeader->width * pHeader->height))
				{
					MFDebug_Warn(2, "Failed loading image (Unexpected end of file)");
					return NULL;
				}

				MFIntTexturePixel pixel;

				pixel.r = (float)pPosition[2] * (1.0f/255.0f);
				pixel.g = (float)pPosition[1] * (1.0f/255.0f);
				pixel.b = (float)pPosition[0] * (1.0f/255.0f);
				if(pHeader->bpp == 32)
					pixel.a = (float)pPosition[3] * (1.0f/255.0f);
				else
					pixel.a = 1.0f;

				for(int i = 0; i < length; i++)
				{
					*pPixel = pixel;
					++pPixel;
				}

				pixelsRead += length;
				pPosition += bytesPerPixel;
			}
			else
			{ // Raw packet
				uint8 length = ((*pPosition) & 0x7F) + 1;

				pPosition += 1;

				if((pPosition + (bytesPerPixel * length)) > pTarga + imageSize)
				{
					MFDebug_Warn(2, "Failed loading image (Unexpected end of file)");
					return NULL;
				}

				if((pixelsRead + length) > (uint32)(pHeader->width * pHeader->height))
				{
					MFDebug_Warn(2, "Failed loading image (Unexpected end of file)");
					return NULL;
				}

				for(int i=0; i<length; i++)
				{
					pPixel->r = (float)pPosition[2] * (1.0f/255.0f);
					pPixel->g = (float)pPosition[1] * (1.0f/255.0f);
					pPixel->b = (float)pPosition[0] * (1.0f/255.0f);
					if(pHeader->bpp == 32)
						pPixel->a = (float)pPosition[3] * (1.0f/255.0f);
					else
						pPixel->a = 1.0f;

					++pPixel;

					pPosition += bytesPerPixel;
				}

				pixelsRead += length;
			}
		}
	}
	else if(pHeader->imageType == 2) // raw RGB
	{
		if((pPosition + (bytesPerPixel * (pHeader->width * pHeader->height))) > pTarga + imageSize)
		{
			MFDebug_Warn(2, "Failed loading image (Unexpected end of file)");
			return NULL;
		}

		if(pHeader->bpp == 16)
		{
			// this doesnt actually seem to be valid even tho the spec says so.
//			uint8 hasAlpha = pHeader->flags & 0xF;
			uint8 hasAlpha = 0;

			for(int a=0; a<pHeader->width*pHeader->height; a++)
			{
				uint16 c = *(uint16*)pImageData;
				uint8 r, g, b;

				r = (c >> 7) & 0xF8;
				r |= r >> 5;
				g = (c >> 2) & 0xF8;
				g |= g >> 5;
				b = (c << 3) & 0xF8;
				b |= b >> 5;

				pPixel->r = (float)r * (1.0f/255.0f);
				pPixel->g = (float)g * (1.0f/255.0f);
				pPixel->b = (float)b * (1.0f/255.0f);
				pPixel->a = hasAlpha ? ((c & 0x8000) ? 1.0f : 0.0f) : 1.0f;

				pImageData += bytesPerPixel;
				++pPixel;
			}
		}
		else if(pHeader->bpp == 24)
		{
			for(int a=0; a<pHeader->width*pHeader->height; a++)
			{
				pPixel->r = (float)pImageData[2] * (1.0f/255.0f);
				pPixel->g = (float)pImageData[1] * (1.0f/255.0f);
				pPixel->b = (float)pImageData[0] * (1.0f/255.0f);
				pPixel->a = 1.0f;
				pImageData += bytesPerPixel;
				++pPixel;
			}
		}
		else if(pHeader->bpp == 32)
		{
			for(int a=0; a<pHeader->width*pHeader->height; a++)
			{
				pPixel->r = (float)pImageData[2] * (1.0f/255.0f);
				pPixel->g = (float)pImageData[1] * (1.0f/255.0f);
				pPixel->b = (float)pImageData[0] * (1.0f/255.0f);
				pPixel->a = (float)pImageData[3] * (1.0f/255.0f);
				pImageData += bytesPerPixel;
				++pPixel;
			}
		}
	}
	else if(pHeader->imageType == 1) // paletted
	{
		MFDebug_Assert(false, "Paletted images not yet supported....");
	}

	if(isSavedFlipped)
	{
		MFIntTexture_FlipImage(pImage);
	}

	return pImage;
}

MFIntTexture* LoadBMP(const void *pMemory, size_t imageSize)
{
	unsigned char *pBMP = (unsigned char *)pMemory;

	if(imageSize < (sizeof(BMPHeader) + 1))
		return NULL;

	if(pBMP[0] != 'B' || pBMP[1] != 'M')
	{
		MFDebug_Warn(2, "Not a bitmap image.");
		return NULL;
	}

	BMPHeader *pHeader = (BMPHeader*)pBMP;
	BMPInfoHeader *pInfoHeader = (BMPInfoHeader*)&pHeader[1];

	unsigned char *pImageData = pBMP + pHeader->offset;

	MFIntTexture *pImage = (MFIntTexture*)MFHeap_Alloc(sizeof(MFIntTexture));

	pImage->numSurfaces = 1;
	pImage->pSurfaces = (MFIntTextureSurface*)MFHeap_Alloc(sizeof(MFIntTextureSurface));

	pImage->pSurfaces[0].pData = (MFIntTexturePixel*)MFHeap_Alloc(sizeof(MFIntTexturePixel)*pInfoHeader->width*pInfoHeader->height);
	pImage->pSurfaces[0].width = pInfoHeader->width;
	pImage->pSurfaces[0].height = pInfoHeader->height;

	MFIntTexturePixel *pPixel = pImage->pSurfaces[0].pData;

	bool isSavedFlipped = true;

	switch(pInfoHeader->compression)
	{
		case BMCT_RGB:
			if(pInfoHeader->bits == 24)
			{
				struct Pixel24
				{
					unsigned char b, g, r;
				};

				Pixel24 *p = (Pixel24*)pImageData;

				for(int y=0; y<pInfoHeader->height; y++)
				{
					for(int x=0; x<pInfoHeader->width; x++)
					{
						pPixel->r = (float)p->r * (1.0f/255.0f);
						pPixel->g = (float)p->g * (1.0f/255.0f);
						pPixel->b = (float)p->b * (1.0f/255.0f);
						pPixel->a = 1.0f;

						++pPixel;
						++p;
					}
				}
			}
			else if(pInfoHeader->bits == 32)
			{
				struct Pixel32
				{
					unsigned char b, g, r, a;
				};

				Pixel32 *p = (Pixel32*)pImageData;

				for(int y=0; y<pInfoHeader->height; y++)
				{
					for(int x=0; x<pInfoHeader->width; x++)
					{
						pPixel->r = (float)p->r * (1.0f/255.0f);
						pPixel->g = (float)p->g * (1.0f/255.0f);
						pPixel->b = (float)p->b * (1.0f/255.0f);
						pPixel->a = (float)p->a * (1.0f/255.0f);

						++pPixel;
						++p;
					}
				}
			}
			else if(pInfoHeader->bits == 8)
			{
				struct Pixel32
				{
					unsigned char b, g, r, a;
				};

				Pixel32 *pPalette = (Pixel32*)((char*)pInfoHeader + pInfoHeader->size);
				uint8 *p = (uint8*)pImageData;

				float alpha = 0.f;

				for(int y=0; y<pInfoHeader->height; y++)
				{
					for(int x=0; x<pInfoHeader->width; x++)
					{
						Pixel32 *pColour = pPalette + *p;

						pPixel->r = (float)pColour->r * (1.0f/255.0f);
						pPixel->g = (float)pColour->g * (1.0f/255.0f);
						pPixel->b = (float)pColour->b * (1.0f/255.0f);
						pPixel->a = alpha = (float)pColour->a * (1.0f/255.0f);

						++pPixel;
						++p;
					}
				}

				// if there was no alpha present in the image... set it all to white
				if(alpha == 0.f)
				{
					pPixel = pImage->pSurfaces[0].pData;

					for(int y=0; y<pInfoHeader->height; y++)
					{
						for(int x=0; x<pInfoHeader->width; x++)
						{
							pPixel->a = 1.0f;
							++pPixel;
						}
					}
				}
			}
			else
			{
				MFDebug_Warn(2, "Unsupported colour depth.");
				return NULL;
			}
			break;

		case BMCT_RLE8:
		case BMCT_RLE4:
		case BMCT_BITFIELDS:
		default:
		{
			MFDebug_Warn(2, "Compressed bitmaps not supported.");
			return NULL;
		}
	}

	if(isSavedFlipped)
	{
		MFIntTexture_FlipImage(pImage);
	}

	return pImage;
}

MFIntTexture* LoadDDS(const void *pMemory, size_t imageSize)
{
	if(imageSize < sizeof(DDS_HEADER))
		return NULL;

	DDS_IMAGE *pDDS = (DDS_IMAGE*)pMemory;
	if(pDDS->magic != MFMAKEFOURCC('D', 'D', 'S', ' ') || pDDS->header.dwSize != sizeof(DDS_HEADER))
	{
		MFDebug_Warn(2, "Not a DDS image.");
		return NULL;
	}

	bool bHasDX10Header = pDDS->header.ddspf.dwFourCC == MFMAKEFOURCC('D', 'X', '1', '0');
	char *pData = bHasDX10Header ? (char*)&pDDS->imageData : (char*)&pDDS->imageData10;

	MFIntTexture *pImage = (MFIntTexture*)MFHeap_Alloc(sizeof(MFIntTexture));

	MFDebug_Assert(false, "Load DDS...");

//	pImage->numSurfaces = 1;
//	pImage->pSurfaces = (MFIntTextureSurface*)MFHeap_Alloc(sizeof(MFIntTextureSurface));

//	pImage->pSurfaces[0].pData = (MFIntTexturePixel*)MFHeap_Alloc(sizeof(MFIntTexturePixel)*pInfoHeader->width*pInfoHeader->height);
//	pImage->pSurfaces[0].width = pInfoHeader->width;
//	pImage->pSurfaces[0].height = pInfoHeader->height;

//	MFIntTexturePixel *pPixel = pImage->pSurfaces[0].pData;

	uint32 width = pDDS->header.dwWidth;
	uint32 height = pDDS->header.dwHeight;

	uint32 numArrayElements = bHasDX10Header ? pDDS->dx10Header.arraySize : 1;

	bool bCubeMap = false;
	uint32 numSurfaces = (bCubeMap ? 6 : 1) * numArrayElements;

	uint32 depthLayerCount = pDDS->header.dwDepth;

	for(uint32 i=0; i<numSurfaces; ++i)
	{
		for(uint32 mip=0; mip<pDDS->header.dwMipMapCount; ++mip)
		{
			uint32 mipWidth = width >> mip;
			uint32 mipHeight = height >> mip;
			mipWidth = mipWidth ? mipWidth : 1;
			mipHeight = mipHeight ? mipHeight : 1;

			// the number of depth layers mip too...
			uint32 depthLayers = depthLayerCount >> mip;
			depthLayers = depthLayers ? depthLayers : 1;

			for(uint32 depth=0; depth<1; ++depth)
			{
				// load image data...
			}
		}
	}

	return pImage;
}

bool IsPowerOf2(int x)
{
	while(x)
	{
		if(x&1)
		{
			x>>=1;
			if(x)
				return false;
		}
		else
			x>>=1;
	}

	return true;
}

void ATICompress(MFIntTexturePixel *pSourceBuffer, int width, int height, MFImageFormat targetFormat, void *pOutputBuffer)
{
#if defined(MF_ENABLE_ATI_COMPRESSOR)
	ATI_TC_FORMAT atiFormat;
	switch(targetFormat)
	{
		case ImgFmt_DXT1:
		case ImgFmt_PSP_DXT1:
		case ImgFmt_PSP_DXT1s:
			atiFormat = ATI_TC_FORMAT_DXT1;
			break;
		case ImgFmt_DXT2:
		case ImgFmt_DXT3:
		case ImgFmt_PSP_DXT3:
		case ImgFmt_PSP_DXT3s:
			atiFormat = ATI_TC_FORMAT_DXT3;
			break;
		case ImgFmt_DXT4:
		case ImgFmt_DXT5:
		case ImgFmt_PSP_DXT5:
		case ImgFmt_PSP_DXT5s:
			atiFormat = ATI_TC_FORMAT_DXT5;
			break;
	}

	// Init source texture
	ATI_TC_Texture srcTexture;
	srcTexture.dwSize = sizeof(srcTexture);
	srcTexture.dwWidth = width;
	srcTexture.dwHeight = height;
	srcTexture.dwPitch = width*sizeof(float);
	srcTexture.format = ATI_TC_FORMAT_ARGB_32F;
	srcTexture.dwDataSize = ATI_TC_CalculateBufferSize(&srcTexture);
	srcTexture.pData = (ATI_TC_BYTE*)pSourceBuffer;

	// Init dest texture
	ATI_TC_Texture destTexture;
	destTexture.dwSize = sizeof(destTexture);
	destTexture.dwWidth = width;
	destTexture.dwHeight = height;
	destTexture.dwPitch = 0;
	destTexture.format = atiFormat;
	destTexture.dwDataSize = ATI_TC_CalculateBufferSize(&destTexture);
	destTexture.pData = (ATI_TC_BYTE*)pOutputBuffer;

	ATI_TC_CompressOptions options;
	options.dwSize = sizeof(options);
	options.bUseChannelWeighting = FALSE;
	options.fWeightingRed = 1.0;			/* Weighting of the Red or X Channel */
	options.fWeightingGreen = 1.0;		/* Weighting of the Green or Y Channel */
	options.fWeightingBlue = 1.0;			/* Weighting of the Blue or Z Channel */
	options.bUseAdaptiveWeighting = TRUE;	/* Adapt weighting on a per-block basis */
	options.bDXT1UseAlpha = TRUE;
	options.nAlphaThreshold = 128;

	// Compress
	ATI_TC_ConvertTexture(&srcTexture, &destTexture, &options, NULL, NULL, NULL);
#else
	// not supported
	MFDebug_Assert(false, "ATI's S3 Texture Compressor not available in this build..");
#endif
}

void Swizzle_PSP(char* out, const char* in, uint32 width, uint32 height, MFImageFormat format)
{
	uint32 blockx, blocky;
	uint32 j;

	// calculate width in bytes
	width = (width * MFImage_GetBitsPerPixel(format)) / 8;

	uint32 width_blocks = (width / 16);
	uint32 height_blocks = (height / 8);

	uint32 src_pitch = (width-16)/4;
	uint32 src_row = width * 8;

	const char* ysrc = in;
	uint32* dst = (uint32*)out;

	for(blocky = 0; blocky < height_blocks; ++blocky)
	{
		const char* xsrc = ysrc;
		for(blockx = 0; blockx < width_blocks; ++blockx)
		{
			const uint32* src = (uint32*)xsrc;
			for(j = 0; j < 8; ++j)
			{
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				*(dst++) = *(src++);
				src += src_pitch;
			}
			xsrc += 16;
		}
		ysrc += src_row;
	}
}

int ConvertSurface(MFIntTextureSurface *pSourceSurface, MFTextureSurfaceLevel *pOutputSurface, MFImageFormat targetFormat, MFPlatform platform)
{
	// convert image...
	int width = pSourceSurface->width;
	int height = pSourceSurface->height;

	int x, y;
	MFIntTexturePixel *pSource = pSourceSurface->pData;

	switch(targetFormat)
	{
		case ImgFmt_A8R8G8B8:
		case ImgFmt_XB_A8R8G8B8s:
		{
			struct PixelBGRA { uint8 b, g, r, a; } *pTarget = (PixelBGRA*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					pTarget->a = (uint8)((int)(pSource->a*255.f) & 0xFF);
					pTarget->r = (uint8)((int)(pSource->r*255.f) & 0xFF);
					pTarget->g = (uint8)((int)(pSource->g*255.f) & 0xFF);
					pTarget->b = (uint8)((int)(pSource->b*255.f) & 0xFF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_A8B8G8R8:
		case ImgFmt_XB_A8B8G8R8s:
		case ImgFmt_PSP_A8B8G8R8s:
		{
			float alphaScale = platform == FP_PS2 ? 128.0f : 255.0f;

			struct PixelRGBA { uint8 r, g, b, a; } *pTarget = (PixelRGBA*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					pTarget->a = (uint8)((int)(pSource->a*alphaScale) & 0xFF);
					pTarget->r = (uint8)((int)(pSource->r*255.0f) & 0xFF);
					pTarget->g = (uint8)((int)(pSource->g*255.0f) & 0xFF);
					pTarget->b = (uint8)((int)(pSource->b*255.0f) & 0xFF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_B8G8R8A8:
		case ImgFmt_XB_B8G8R8A8s:
		{
			struct PixelARGB { uint8 a, r, g, b; } *pTarget = (PixelARGB*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					pTarget->a = (uint8)((int)(pSource->a*255.f) & 0xFF);
					pTarget->r = (uint8)((int)(pSource->r*255.f) & 0xFF);
					pTarget->g = (uint8)((int)(pSource->g*255.f) & 0xFF);
					pTarget->b = (uint8)((int)(pSource->b*255.f) & 0xFF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_R8G8B8A8:
		case ImgFmt_XB_R8G8B8A8s:
		{
			struct PixelABGR { uint8 a, b, g, r; } *pTarget = (PixelABGR*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					pTarget->a = (uint8)((int)(pSource->a*255.f) & 0xFF);
					pTarget->r = (uint8)((int)(pSource->r*255.f) & 0xFF);
					pTarget->g = (uint8)((int)(pSource->g*255.f) & 0xFF);
					pTarget->b = (uint8)((int)(pSource->b*255.f) & 0xFF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_R8G8B8:
		{
			struct PixelBGR { uint8 b, g, r; } *pTarget = (PixelBGR*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					pTarget->r = (uint8)((int)(pSource->r*255.f) & 0xFF);
					pTarget->g = (uint8)((int)(pSource->g*255.f) & 0xFF);
					pTarget->b = (uint8)((int)(pSource->b*255.f) & 0xFF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_B8G8R8:
		{
			struct PixelRGB { uint8 r, g, b; } *pTarget = (PixelRGB*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					pTarget->r = (uint8)((int)(pSource->r) & 0xFF);
					pTarget->g = (uint8)((int)(pSource->g) & 0xFF);
					pTarget->b = (uint8)((int)(pSource->b) & 0xFF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_A2R10G10B10:
		{
			uint32 *pTarget = (uint32*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint32)(pSource->a*3.0f) & 0x3) << 30 |
								((uint32)(pSource->r*1023.0f) & 0x3FF) << 20 |
								((uint32)(pSource->g*1023.0f) & 0x3FF) << 10 |
								((uint32)(pSource->b*1023.0f) & 0x3FF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_A2B10G10R10:
		{
			uint32 *pTarget = (uint32*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint32)(pSource->a*3.0f) & 0x3) << 30 |
								((uint32)(pSource->b*1023.0f) & 0x3FF) << 20 |
								((uint32)(pSource->g*1023.0f) & 0x3FF) << 10 |
								((uint32)(pSource->r*1023.0f) & 0x3FF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_A16B16G16R16:
		{
			uint64 *pTarget = (uint64*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint64)(pSource->a*65535.0f) & 0xFFFF) << 48 |
								((uint64)(pSource->b*65535.0f) & 0xFFFF) << 32 |
								((uint64)(pSource->g*65535.0f) & 0xFFFF) << 16 |
								((uint64)(pSource->r*65535.0f) & 0xFFFF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_R5G6B5:
		case ImgFmt_XB_R5G6B5s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->r*31.0f) & 0x1F) << 11 |
								((uint16)(pSource->g*63.0f) & 0x3F) << 5 |
								((uint16)(pSource->b*31.0f) & 0x1F);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_R6G5B5:
		case ImgFmt_XB_R6G5B5s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->r*63.0f) & 0x3F) << 10 |
								((uint16)(pSource->g*31.0f) & 0x1F) << 5 |
								((uint16)(pSource->b*31.0f) & 0x1F);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_B5G6R5:
		case ImgFmt_PSP_B5G6R5s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->b*31.0f) & 0x1F) << 11 |
								((uint16)(pSource->g*63.0f) & 0x3F) << 5 |
								((uint16)(pSource->r*31.0f) & 0x1F);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_A1R5G5B5:
		case ImgFmt_XB_A1R5G5B5s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->a*1.0f) & 0x1) << 15 |
								((uint16)(pSource->r*31.0f) & 0x1F) << 10 |
								((uint16)(pSource->g*31.0f) & 0x1F) << 5 |
								((uint16)(pSource->b*31.0f) & 0x1F);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_R5G5B5A1:
		case ImgFmt_XB_R5G5B5A1s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->a*1.0f) & 0x1) |
								((uint16)(pSource->r*31.0f) & 0x1F) << 11 |
								((uint16)(pSource->g*31.0f) & 0x1F) << 6 |
								((uint16)(pSource->b*31.0f) & 0x1F) << 1;
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_A1B5G5R5:
		case ImgFmt_PSP_A1B5G5R5s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->a*1.0f) & 0x1) << 15 |
								((uint16)(pSource->b*31.0f) & 0x1F) << 10 |
								((uint16)(pSource->g*31.0f) & 0x1F) << 5 |
								((uint16)(pSource->r*31.0f) & 0x1F);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_A4R4G4B4:
		case ImgFmt_XB_A4R4G4B4s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->a*15.0f) & 0xF) << 12 |
								((uint16)(pSource->r*15.0f) & 0xF) << 8 |
								((uint16)(pSource->g*15.0f) & 0xF) << 4 |
								((uint16)(pSource->b*15.0f) & 0xF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_A4B4G4R4:
		case ImgFmt_PSP_A4B4G4R4s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->a*15.0f) & 0xF) << 12 |
								((uint16)(pSource->b*15.0f) & 0xF) << 8 |
								((uint16)(pSource->g*15.0f) & 0xF) << 4 |
								((uint16)(pSource->r*15.0f) & 0xF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_R4G4B4A4:
		case ImgFmt_XB_R4G4B4A4s:
		{
			uint16 *pTarget = (uint16*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = ((uint16)(pSource->r*15.0f) & 0xF) << 12 |
								((uint16)(pSource->g*15.0f) & 0xF) << 8 |
								((uint16)(pSource->b*15.0f) & 0xF) << 4 |
								((uint16)(pSource->a*15.0f) & 0xF);
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_ABGR_F16:
		{
			uint64 *pTarget = (uint64*)pOutputSurface->pImageData;
			uint32 c;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					c = (uint32&)pSource->a;
					*pTarget = (uint64)((c & 0xFC000000) >> 16 | (c & 0x007FC000) >> 13) << 48;
					c = (uint32&)pSource->b;
					*pTarget |= (uint64)((c & 0xFC000000) >> 16 | (c & 0x007FC000) >> 13) << 32;
					c = (uint32&)pSource->g;
					*pTarget |= (uint64)((c & 0xFC000000) >> 16 | (c & 0x007FE000) >> 13) << 16;
					c = (uint32&)pSource->r;
					*pTarget |= (uint64)((c & 0xFC000000) >> 16 | (c & 0x007FC000) >> 13);

					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_ABGR_F32:
		{
			float *pTarget = (float*)pOutputSurface->pImageData;

			for(y=0; y<height; y++)
			{
				for(x=0; x<width; x++)
				{
					*pTarget = pSource->r;
					++pTarget;
					*pTarget = pSource->g;
					++pTarget;
					*pTarget = pSource->b;
					++pTarget;
					*pTarget = pSource->a;
					++pTarget;
					++pSource;
				}
			}
			break;
		}

		case ImgFmt_DXT1:
		case ImgFmt_DXT2:
		case ImgFmt_DXT3:
		case ImgFmt_DXT4:
		case ImgFmt_DXT5:
		case ImgFmt_PSP_DXT1:
		case ImgFmt_PSP_DXT3:
		case ImgFmt_PSP_DXT5:
		case ImgFmt_PSP_DXT1s:
		case ImgFmt_PSP_DXT3s:
		case ImgFmt_PSP_DXT5s:
		{
			ATICompress(pSource, width, height, targetFormat, pOutputSurface->pImageData);

			if(targetFormat == ImgFmt_PSP_DXT1 || targetFormat == ImgFmt_PSP_DXT3 ||  targetFormat == ImgFmt_PSP_DXT5 ||
				targetFormat == ImgFmt_PSP_DXT1s || targetFormat == ImgFmt_PSP_DXT3s ||  targetFormat == ImgFmt_PSP_DXT5s)
			{
				// we need to swizzle the PSP buffer about a bit...
			}
			break;
		}

		default:
		{
			MFDebug_Assert(false, MFStr("Conversion for target format '%s' not yet supported...\n", MFImage_GetFormatString(targetFormat)));
			return 1;
		}
	}

	// test for swizzled format..
	if(targetFormat >= ImgFmt_XB_A8R8G8B8s)
	{
		uint32 imageBytes = (width * height * MFImage_GetBitsPerPixel(targetFormat)) / 8;

		char *pBuffer = (char*)MFHeap_Alloc(imageBytes);

#if 0
		uint32 bytesperpixel = MFImage_GetBitsPerPixel(targetFormat) / 8;

		if(targetFormat >= ImgFmt_XB_A8R8G8B8s && targetFormat <= ImgFmt_XB_R4G4B4A4s)
		{
			// swizzle for xbox
			// TODO: Swizzle here.. But we'll swizzle at runtime for the time being....
//			XGSwizzleRect(pOutputSurface->pImageData, 0, NULL, pBuffer, width, height, NULL, bytesperpixel);
			MFCopyMemory(pBuffer, pOutputSurface->pImageData, width*height*bytesperpixel);
		}
		else
#endif
		if(targetFormat >= ImgFmt_PSP_A8B8G8R8s && targetFormat <= ImgFmt_PSP_DXT5s)
		{
			// swizzle for PSP
			Swizzle_PSP(pBuffer, pOutputSurface->pImageData, width, height, targetFormat);
		}

		MFCopyMemory(pOutputSurface->pImageData, pBuffer, imageBytes);
		MFHeap_Free(pBuffer);
	}

	return 0;
}

void PremultiplyAlpha(MFIntTexture *pImage)
{
	MFIntTexturePixel *pPx = pImage->pSurfaces[0].pData;

	for(int s=0; s<pImage->numSurfaces; ++s)
	{
		for(int a=0; a<pImage->pSurfaces[s].width; a++)
		{
			for(int a=0; a<pImage->pSurfaces[s].height; a++)
			{
				pPx->r *= pPx->a;
				pPx->g *= pPx->a;
				pPx->b *= pPx->a;
				++pPx;
			}
		}
	}
}

MF_API MFIntTexture *MFIntTexture_CreateFromFile(const char *pFilename)
{
	// find format
	const char *pExt = MFString_GetFileExtension(pFilename);

	MFIntTextureFormat format = MFITF_Max;
	for(int a=0; a<gNumFileExtensions; ++a)
	{
		if(!MFString_Compare(pExt, gFileExtensions[a]))
		{
			format = gFileTypeMap[a];
			break;
		}
	}
	if(format == MFITF_Max)
		return NULL;

	// load file
	size_t size;
	char *pData = MFFileSystem_Load(pFilename, &size);
	if(!pData)
		return NULL;

	// load the image
	MFIntTexture *pImage = MFIntTexture_CreateFromFileInMemory(pData, size, format);

	// free file
	MFHeap_Free(pData);

	return pImage;
}

MF_API MFIntTexture *MFIntTexture_CreateFromFileInMemory(const void *pMemory, size_t size, MFIntTextureFormat format)
{
	MFIntTexture *pImage = NULL;

	switch(format)
	{
		case MFITF_TGA:
			pImage = LoadTGA(pMemory, size);
			break;
		case MFITF_BMP:
			pImage = LoadBMP(pMemory, size);
			break;
		case MFITF_PNG:
#if defined(MF_ENABLE_PNG)
			pImage = LoadPNG(pMemory, size);
#else
			MFDebug_Assert(false, "PNG support is not enabled in this build.");
#endif
			break;
		case MFITF_DDS:
			pImage = LoadDDS(pMemory, size);
			break;
		default:
			MFDebug_Assert(false, "Unsupported image format.");
	}

	if(pImage)
	{
		// scan for alpha information
		MFIntTexture_ScanImage(pImage);

		// build the mip chain
//		if(pImage->numSurfaces == 1)
//			MFIntTexture_FilterMipMaps(pImage, 0, 0);
	}

	return pImage;
}

MF_API void MFIntTexture_Destroy(MFIntTexture *pTexture)
{
	for(int a=0; a<pTexture->numSurfaces; a++)
	{
		if(pTexture->pSurfaces[a].pData)
			MFHeap_Free(pTexture->pSurfaces[a].pData);
	}

	if(pTexture->pSurfaces)
		MFHeap_Free(pTexture->pSurfaces);

	if(pTexture)
		MFHeap_Free(pTexture);
}

MF_API MFImageFormat ChooseBestFormat(MFIntTexture *pTexture, MFPlatform platform)
{
	MFImageFormat targetFormat = ImgFmt_A8B8G8R8;

	// choose target format..
	switch(platform)
	{
		case FP_Windows:
		case FP_Linux:
		case FP_OSX:
/*
			if(pImage->opaque || (pImage->oneBitAlpha && premultipliedAlpha))
				targetFormat = ImgFmt_DXT1;
			else
				targetFormat = ImgFmt_DXT5;
*/
			if(pTexture->opaque)
				targetFormat = ImgFmt_A8R8G8B8; //ImgFmt_R5G6B5;
			else if(pTexture->oneBitAlpha)
				targetFormat = ImgFmt_A8R8G8B8; //ImgFmt_A1R5G5B5;
			else
				targetFormat = ImgFmt_A8R8G8B8;
			break;

		case FP_IPhone:
			if(pTexture->opaque)
				targetFormat = ImgFmt_R5G6B5;
			else if(pTexture->oneBitAlpha)
				targetFormat = ImgFmt_R5G5B5A1;
			else
				targetFormat = ImgFmt_R4G4B4A4;
			break;

		case FP_XBox:
			if(pTexture->opaque)
				targetFormat = ImgFmt_XB_R5G6B5s;
			else if(pTexture->oneBitAlpha)
				targetFormat = ImgFmt_XB_A1R5G5B5s;
			else
				targetFormat = ImgFmt_XB_A8R8G8B8s;
			break;

		case FP_PSP:
			if(pTexture->opaque)
				targetFormat = ImgFmt_PSP_B5G6R5s;
			else if(pTexture->oneBitAlpha)
				targetFormat = ImgFmt_PSP_A1B5G5R5s;
			else
				targetFormat = ImgFmt_PSP_A4B4G4R4s;
			break;

		case FP_PS2:
			if(pTexture->opaque || pTexture->oneBitAlpha)
				targetFormat = ImgFmt_A1B5G5R5;
			else
				targetFormat = ImgFmt_A8B8G8R8;
			break;

		default:
			break;
	}

	return targetFormat;
}

MF_API void MFIntTexture_CreateRuntimeData(MFIntTexture *pTexture, MFTextureTemplateData **ppTemplateData, size_t *pSize, MFPlatform platform, uint32 flags, MFImageFormat targetFormat)
{
	*ppTemplateData = NULL;
	if(pSize)
		*pSize = 0;

	// choose target image format
	if(targetFormat == ImgFmt_Unknown)
		targetFormat = ChooseBestFormat(pTexture, platform);

	// check minimum pitch
	MFDebug_Assert((pTexture->pSurfaces[0].width*MFImage_GetBitsPerPixel(targetFormat)) / 8 >= 16, "Textures should have a minimum pitch of 16 bytes.");

	// check power of 2 dimensions
//	MFDebug_Assert(IsPowerOf2(pTexture->pSurfaces[0].width) && IsPowerOf2(pTexture->pSurfaces[0].height), "Texture dimensions are not a power of 2.");

	// check dimensions are a multiple of 4
	MFDebug_Assert((pTexture->pSurfaces[0].width & 0x3) == 0 && (pTexture->pSurfaces[0].height & 3) == 0, "Texture dimensions are not multiples of 4.");

	// begin processing...
	if(flags & MFITF_PreMultipliedAlpha)
		PremultiplyAlpha(pTexture);

	// calculate texture data size..
	size_t imageBytes = MFALIGN(sizeof(MFTextureTemplateData) + sizeof(MFTextureSurfaceLevel)*pTexture->numSurfaces, 0x100);

	for(int a=0; a<pTexture->numSurfaces; a++)
	{
		imageBytes += (pTexture->pSurfaces[a].width * pTexture->pSurfaces[a].height * MFImage_GetBitsPerPixel(targetFormat)) / 8;

		// add palette
		uint32 paletteBytes = 0;

		if(targetFormat == ImgFmt_I8)
			paletteBytes = 4*256;
		if(targetFormat == ImgFmt_I4)
			paletteBytes = 4*16;

		imageBytes += paletteBytes;
	}

	// allocate buffer
//	MFHeap_SetAllocAlignment(4096);
	char *pOutputBuffer = (char*)MFHeap_Alloc(imageBytes);

	MFTextureTemplateData *pTemplate = (MFTextureTemplateData*)pOutputBuffer;
	MFZeroMemory(pTemplate, sizeof(MFTextureTemplateData));

	pTemplate->magicNumber = MFMAKEFOURCC('F','T','E','X');

	pTemplate->imageFormat = targetFormat;

	if(targetFormat >= ImgFmt_XB_A8R8G8B8s)
		pTemplate->flags |= TEX_Swizzled;

	if(!pTexture->opaque)
	{
		if(pTexture->oneBitAlpha)
			pTemplate->flags |= 3;
		else
			pTemplate->flags |= 1;
	}

	if(flags & MFITF_PreMultipliedAlpha)
		pTemplate->flags |= TEX_PreMultipliedAlpha;

	pTemplate->mipLevels = pTexture->numSurfaces;
	pTemplate->pSurfaces = (MFTextureSurfaceLevel*)(pOutputBuffer + sizeof(MFTextureTemplateData));

	MFTextureSurfaceLevel *pSurfaceLevels = (MFTextureSurfaceLevel*)(pOutputBuffer + sizeof(MFTextureTemplateData));

	char *pDataPointer = pOutputBuffer + MFALIGN(sizeof(MFTextureTemplateData) + sizeof(MFTextureSurfaceLevel)*pTexture->numSurfaces, 0x100);

	for(int a=0; a<pTexture->numSurfaces; a++)
	{
		MFZeroMemory(&pSurfaceLevels[a], sizeof(MFTextureSurfaceLevel));

		pSurfaceLevels[a].width = pTexture->pSurfaces[a].width;
		pSurfaceLevels[a].height = pTexture->pSurfaces[a].height;
		pSurfaceLevels[a].bitsPerPixel = MFImage_GetBitsPerPixel(targetFormat);

		pSurfaceLevels[a].xBlocks = -1;
		pSurfaceLevels[a].yBlocks = -1;
		pSurfaceLevels[a].bitsPerBlock = -1;

		pSurfaceLevels[a].pImageData = pDataPointer;
		pSurfaceLevels[a].bufferLength = (pTexture->pSurfaces[a].width*pTexture->pSurfaces[a].height * MFImage_GetBitsPerPixel(targetFormat)) / 8;
		pDataPointer += pSurfaceLevels[a].bufferLength;

		uint32 paletteBytes = 0;

		if(targetFormat == ImgFmt_I8)
			paletteBytes = 4*256;
		if(targetFormat == ImgFmt_I4)
			paletteBytes = 4*16;

		if(paletteBytes)
		{
			pSurfaceLevels[a].pImageData = pDataPointer;
			pSurfaceLevels[a].paletteBufferLength = paletteBytes;
			pDataPointer += paletteBytes;
		}

		// convert surface
		ConvertSurface(&pTexture->pSurfaces[a], &pSurfaceLevels[a], targetFormat, platform);
	}

	// fix up pointers
	for(int a=0; a<pTemplate->mipLevels; a++)
	{
		MFFixUp(pTemplate->pSurfaces[a].pImageData, pOutputBuffer, 0);
		MFFixUp(pTemplate->pSurfaces[a].pPaletteEntries, pOutputBuffer, 0);
	}
	MFFixUp(pTemplate->pSurfaces, pOutputBuffer, 0);

	*ppTemplateData = (MFTextureTemplateData*)pOutputBuffer;
	if(pSize)
		*pSize = imageBytes;
}

MF_API void MFIntTexture_WriteToHeaderFile(MFIntTexture *pTexture, const char *pFilename)
{

}

MF_API void MFIntTexture_FilterMipMaps(MFIntTexture *pTexture, int numMipLevels, uint32 mipFilterOptions)
{
	struct MipLevels
	{
		int width, height;
	} levels[128];
	int numLevels = 1;

	levels[0].width = pTexture->pSurfaces[0].width;
	levels[0].height = pTexture->pSurfaces[0].height;

	while(levels[numLevels-1].width > 1 && levels[numLevels-1].height > 1)
	{
		levels[numLevels].width = levels[numLevels-1].width;
		levels[numLevels].height = levels[numLevels-1].height;
		if(levels[numLevels].width > 1)
			levels[numLevels].width >>= 1;
		if(levels[numLevels].height > 1)
			levels[numLevels].height >>= 1;
		++numLevels;
	}

	if(numMipLevels > 0)
		numLevels = MFMin(numLevels, numMipLevels);

	if(numLevels == 1)
		return;

	pTexture->pSurfaces = (MFIntTextureSurface*)MFHeap_Realloc(pTexture->pSurfaces, sizeof(MFIntTextureSurface)*numLevels);
	pTexture->numSurfaces = numLevels;

	for(int l = 1; l < numLevels; ++l)
	{
		pTexture->pSurfaces[l].width = levels[l].width;
		pTexture->pSurfaces[l].height = levels[l].height;
		pTexture->pSurfaces[l].pData = (MFIntTexturePixel*)MFHeap_Alloc(sizeof(MFIntTexturePixel)*levels[l].width*levels[l].height);

		MFScaleImage scaleData;
		scaleData.algorithm = SA_Box;
		scaleData.format = ImgFmt_ABGR_F32;
		scaleData.pSourceImage = pTexture->pSurfaces[l-1].pData;
		scaleData.sourceWidth = pTexture->pSurfaces[l-1].width;
		scaleData.sourceHeight = pTexture->pSurfaces[l-1].height;
		scaleData.sourceStride = pTexture->pSurfaces[l-1].width;
		scaleData.pTargetBuffer = pTexture->pSurfaces[l].pData;
		scaleData.targetWidth = pTexture->pSurfaces[l].width;
		scaleData.targetHeight = pTexture->pSurfaces[l].height;
		scaleData.targetStride = pTexture->pSurfaces[l].width;

		MFImage_Scale(&scaleData);
	}
}

MF_API void MFIntTexture_FilterMipMap(MFIntTexturePixel *pSource, MFIntTexturePixel *pDest, int destWidth, int destHeight, uint32 mipFilterOptions)
{
	// TODO: this naive filter doesn't work properly with non-power-of-2 textures!
	// the right and lower edges of the image will be trimmed

	int sourceWidth = destWidth*2;
	for(int y = 0, sy = 0; y < destHeight; ++y, sy += 2)
	{
		for(int x = 0, sx = 0; x < destWidth; ++x, sx += 2)
		{
			int sourceOffset = sx + sy*sourceWidth;
			MFIntTexturePixel &tl = pSource[sourceOffset];
			MFIntTexturePixel &tr = pSource[sourceOffset + 1];
			MFIntTexturePixel &bl = pSource[sourceOffset + sourceWidth];
			MFIntTexturePixel &br = pSource[sourceOffset + 1 + sourceWidth];
			MFIntTexturePixel &result = pDest[x + y*destWidth];

			result.r = (tl.r + tr.r + bl.r + br.r) * 0.25f;
			result.g = (tl.g + tr.g + bl.g + br.g) * 0.25f;
			result.b = (tl.b + tr.b + bl.b + br.b) * 0.25f;
			result.a = (tl.a + tr.a + bl.a + br.a) * 0.25f;
		}
	}
}

MF_API void MFIntTexture_ScanImage(MFIntTexture *pTexture)
{
	pTexture->opaque = true;
	pTexture->oneBitAlpha = true;

	for(int a=0; a<pTexture->numSurfaces; a++)
	{
		for(int b=0; b<pTexture->pSurfaces[a].width; b++)
		{
			for(int c=0; c<pTexture->pSurfaces[a].height; c++)
			{
				float alpha = pTexture->pSurfaces[a].pData[c*pTexture->pSurfaces[a].width + b].a;

				if(alpha != 1.0f)
				{
					pTexture->opaque = false;

					if(alpha != 0.0f)
						pTexture->oneBitAlpha = false;
				}
			}
		}
	}
}

MF_API void MFIntTexture_FlipImage(MFIntTexture *pTexture)
{
	for(int a=0; a<pTexture->numSurfaces; a++)
	{
		int halfHeight = pTexture->pSurfaces[a].height / 2;
		int stride = pTexture->pSurfaces[a].width * sizeof(MFIntTexturePixel);
		int height = pTexture->pSurfaces[a].height;

		char *pBuffer = (char*)MFHeap_Alloc(stride);
		char *pData = (char*)pTexture->pSurfaces[a].pData;

		for(int b=0; b<halfHeight; b++)
		{
			// swap lines
			MFCopyMemory(pBuffer, &pData[b*stride], stride);
			MFCopyMemory(&pData[b*stride], &pData[(height-b-1)*stride], stride);
			MFCopyMemory(&pData[(height-b-1)*stride], pBuffer, stride);
		}

		MFHeap_Free(pBuffer);
	}
}
