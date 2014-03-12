#if !defined(_MFANIMATION_INTERNAL_H)
#define _MFANIMATION_INTERNAL_H

#include "MFModel_Internal.h"
#include "MFAnimation.h"

struct MFAnimationTemplate;

MFInitStatus MFAnimation_InitModule(int moduleId, bool bPerformInitialisation);
void MFAnimation_DeinitModule();

struct MFAnimationCurrentFrame
{
	float frameTime;

	int tweenStart, tweenEnd;
	float tween;
};

struct MFAnimationBlendLayer
{
	float frameTime;

	MFAnimationCurrentFrame *pCurFrames;
};

struct MFAnimation
{
	MFAnimationTemplate *pTemplate;

	MFModel *pModel;
	MFModelBone *pBones;
	uint32 numBones;

	int *pBoneMap;

	MFMatrix *pMatrices;
	MFMatrix *pCustomMatrices;

	MFAnimationBlendLayer blendLayer;
};

struct MFAnimationFrame
{
	MFMatrix key;
//	MFQuaternion rot;
//	MFVector scale;
//	MFVector trans;
};

struct MFAnimationBone
{
	const char *pBoneName;

	float *pTime;
	MFAnimationFrame *pFrames;
	uint32 numFrames;
};

struct MFAnimationTemplate : MFResource
{
	const char *pAnimName;

	MFAnimationBone *pBones;
	uint32 numBones;

	float startTime, endTime;
};

#endif
