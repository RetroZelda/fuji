#if !defined(_MFMODEL_INTERNAL_H)
#define _MFMODEL_INTERNAL_H

#include "MFBoundingVolume.h"
#include "MFModel.h"
#include "MFMaterial_Internal.h"
#include "MFAnimation.h"
#include "MFResource.h"

struct MFModelTemplate;

MFInitStatus MFModel_InitModule(int moduleId, bool bPerformInitialisation);
void MFModel_DeinitModule();

void MFModel_InitModulePlatformSpecific();
void MFModel_DeinitModulePlatformSpecific();

void MFModel_CreateMeshChunk(MFMeshChunk *pMeshChunk);
void MFModel_DestroyMeshChunk(MFMeshChunk *pMeshChunk);
void MFModel_FixUpMeshChunk(MFMeshChunk *pMeshChunk, void *pBase, bool load);

void* MFModelInternal_PendingAnimationTemplate(size_t *pSize);

enum MFModelDataChunkType
{
	MFChunkType_Unknown = -1,

	MFChunkType_SubObjects = 0,
	MFChunkType_Bones,
	MFChunkType_Collision,
	MFChunkType_Tags,
	MFChunkType_Materials,
	MFChunkType_BinaryFilesystem,

	MFChunkType_Max,
	MFChunkType_ForceInt = 0x7FFFFFFF
};

struct MFModelDataChunk
{
	MFModelDataChunkType chunkType;
	int count;
	void *pData;
	uint32 reserved;
};

MFModelDataChunk* MFModel_GetDataChunk(MFModelTemplate *pModelTemplate, MFModelDataChunkType chunkID);
MFMeshChunk* MFModel_GetMeshChunkInternal(MFModelTemplate *pModelTemplate, int subobjectIndex, int meshChunkIndex);

struct MFModel
{
	MFMatrix worldMatrix;

	MFModelTemplate *pTemplate;
	MFAnimation *pAnimation;

	MFStateBlock *pEntityState;

	// subobject flags
	// * enabled flags
	// * subobject colours
};

struct MFModelTemplate : MFResource
{
	const char *pModelName;

	MFModelDataChunk *pDataChunks;
	int numDataChunks;

	MFBoundingVolume boundingVolume;
};

struct MFModelSubObject
{
	const char *pSubObjectName;
//	MFMaterial *pMaterial;

	MFMeshChunk *pMeshChunks;
	int numMeshChunks;

	int subobjectAnimMatrix;
};

struct MFModelBone
{
	MFMatrix boneMatrix;
	MFMatrix worldMatrix;
	MFMatrix invWorldMatrix;
	const char *pBoneName;
	const char *pParentName;
	int16 *pChildren;
	int16 numChildren;
	int16 parent;
};

struct MFModelTag
{
	MFMatrix tagMatrix;
	const char *pTagName;

	uint32 reserved[3];
};

struct BinaryFilesystemChunk
{
	// this is a binary archive that can be mounted and accessed with the memory filesystem..
	// it could contain texture data, or other relevant model related data..
};

#endif
