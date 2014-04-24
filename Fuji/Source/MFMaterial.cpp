/**** Defines ****/

/**** Includes ****/

#include "Fuji_Internal.h"
#include "MFDisplay.h"
#include "MFTexture_Internal.h"
#include "MFMaterial_Internal.h"
#include "Materials/MFMat_Standard.h"
#include "MFFileSystem.h"
#include "MFIni.h"
#include "MFPtrList.h"
#include "MFSystem.h"
#include "MFRenderState.h"

#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFInput.h"

#include "MFDriver.h"

#include "SysLogo256.h"
#include "SysLogo64.h"
#if defined(_PSP)
#include "connected.h"
#include "disconnected.h"
#include "power.h"
#include "charging.h"
#include "usb_icon.h"
#endif

/**** internal functions ****/

void		MaterialInternal_Update(MFMaterial *pMaterial);
const char*	MaterialInternal_GetIDString(MFMaterial *pMaterial);

void		MaterialInternal_InitialiseFromDefinition(MFIni *pDefIni, MFMaterial *pMat, const char *pDefinition);


/**** Globals ****/

struct MaterialDefinition
{
	const char *pName;
	MFIni *pIni;
	bool ownsIni;

	MaterialDefinition *pNextDefinition;
};

void MFMaterial_DestroyDefinition(MaterialDefinition *pDefinition);

MFPtrListDL<MaterialDefinition> gMaterialDefList;
MaterialDefinition *pDefinitionRegistry = NULL;

MFPtrList<MFMaterialType> gMaterialRegistry;

MFMaterial *pCurrentMaterial = NULL;

MFMaterial *pNoneMaterial = NULL;
MFMaterial *pWhiteMaterial = NULL;
MFMaterial *pSysLogoLarge = NULL;
MFMaterial *pSysLogoSmall = NULL;
MFMaterial *pConnected = NULL;
MFMaterial *pDisconnected = NULL;
MFMaterial *pPower = NULL;
MFMaterial *pCharging = NULL;
MFMaterial *pUSB = NULL;

void MFMat_Standard_Register();
void MFMat_Effect_Register();

MaterialBrowser matBrowser;

/**** Functions ****/

static void MFMaterial_Destroy(MFResource *pRes)
{
	MFMaterial *pMaterial = (MFMaterial*)pRes;

	MFStateBlock_Destroy(pMaterial->pMaterialState);

	pMaterial->pType->materialCallbacks.pDestroyInstance(pMaterial);

	MFHeap_Free(pMaterial->pInstanceData);
	MFHeap_Free(pMaterial);
}

MFInitStatus MFMaterial_InitModule(int moduleId, bool bPerformInitialisation)
{
	MFCALLSTACK;

	MFRT_Material = MFResource_Register("MFMaterial", &MFMaterial_Destroy);

	gMaterialRegistry.Init("Material Registry", gDefaults.material.maxMaterialTypes);
	gMaterialDefList.Init("Material Definitions List", gDefaults.material.maxMaterialDefs);

	DebugMenu_AddItem("Material Browser", "Fuji Options", &matBrowser);

	MFMat_Standard_Register();
	MFMat_Effect_Register();

	if(MFFileSystem_Exists("Materials.ini") && MFMaterial_AddDefinitionsFile("Materials", "Materials"))
	{
		MFDebug_Warn(3, "Failed to load Materials.ini");
	}

#if MF_RENDERER == MF_DRIVER_D3D11 || defined(MF_RENDERPLUGIN_D3D11)
	// HACK?
	MFTexture *pSysLogoLargeTexture = MFTexture_Create("_None");
	MFTexture *pSysLogoSmallTexture = MFTexture_Create("_None");
#else
	// create the logo textures from raw data
	MFTexture *pSysLogoLargeTexture = MFTexture_CreateFromRawData("SysLogoLarge", SysLogo256_data, SysLogo256_width, SysLogo256_height, (MFImageFormat)SysLogo256_format, SysLogo256_flags);
	MFTexture *pSysLogoSmallTexture = MFTexture_CreateFromRawData("SysLogoSmall", SysLogo64_data, SysLogo64_width, SysLogo64_height, (MFImageFormat)SysLogo64_format, SysLogo64_flags);
#endif

	// create standard materials
	pNoneMaterial = MFMaterial_Create("_None");
	pWhiteMaterial = MFMaterial_Create("_White");
	pSysLogoLarge = MFMaterial_Create("SysLogoLarge");
	pSysLogoSmall = MFMaterial_Create("SysLogoSmall");

	// disable backface cullign on the default materials
	MFMaterial_SetParameterI(pNoneMaterial, MFMatStandard_CullMode, 0, MFMatStandard_Cull_None);
	MFMaterial_SetParameterI(pWhiteMaterial, MFMatStandard_CullMode, 0, MFMatStandard_Cull_None);

	// release a reference to the logo textures
	MFTexture_Release(pSysLogoLargeTexture);
	MFTexture_Release(pSysLogoSmallTexture);

#if defined(_PSP)
	// create PSP specific stock materials
	MFTexture *pConnectedTexture = MFTexture_CreateFromRawData("connected", connected_data, connected_width, connected_height, (MFImageFormat)connected_format, connected_flags);
	MFTexture *pDisconnectedTexture = MFTexture_CreateFromRawData("disconnected", disconnected_data, disconnected_width, disconnected_height, (MFImageFormat)disconnected_format, disconnected_flags);
	MFTexture *pPowerTexture = MFTexture_CreateFromRawData("power", power_data, power_width, power_height, (MFImageFormat)power_format, power_flags);
	MFTexture *pChargingTexture = MFTexture_CreateFromRawData("charging", charging_data, charging_width, charging_height, (MFImageFormat)charging_format, charging_flags);
	MFTexture *pUSBTexture = MFTexture_CreateFromRawData("usb_icon", usb_icon_data, usb_icon_width, usb_icon_height, (MFImageFormat)usb_icon_format, usb_icon_flags);

	pConnected = MFMaterial_Create("connected");
	pDisconnected = MFMaterial_Create("disconnected");
	pPower = MFMaterial_Create("power");
	pCharging = MFMaterial_Create("charging");
	pUSB = MFMaterial_Create("usb_icon");

	MFTexture_Destroy(pConnectedTexture);
	MFTexture_Destroy(pDisconnectedTexture);
	MFTexture_Destroy(pPowerTexture);
	MFTexture_Destroy(pChargingTexture);
	MFTexture_Destroy(pUSBTexture);
#endif

	return MFIS_Succeeded;
}

void MFMaterial_DeinitModule()
{
	MFCALLSTACK;

	// destroy stock materials
	MFMaterial_Release(pNoneMaterial);
	MFMaterial_Release(pWhiteMaterial);
	MFMaterial_Release(pSysLogoLarge);
	MFMaterial_Release(pSysLogoSmall);

#if defined(_PSP)
	// destroy PSP specific stock materials
	MFMaterial_Release(pConnected);
	MFMaterial_Release(pDisconnected);
	MFMaterial_Release(pPower);
	MFMaterial_Release(pCharging);
	MFMaterial_Release(pUSB);
#endif

	MaterialDefinition *pDef = pDefinitionRegistry;

	while(pDef)
	{
		MaterialDefinition *pNext = pDef->pNextDefinition;
		MFMaterial_DestroyDefinition(pDef);
		pDef = pNext;
	}

	bool bShowHeader = true;

	// list all non-freed materials...
	MFResourceIterator *pI = MFResource_EnumerateFirst(MFRT_Material);
	while(pI)
	{
		if(bShowHeader)
		{
			bShowHeader = false;
			MFDebug_Message("\nUn-freed materials:\n----------------------------------------------------------");
		}

		MFMaterial *pMat = (MFMaterial*)MFResource_Get(pI);

		MFDebug_Message(MFStr("'%s' - x%d", pMat->pName, pMat->refCount));

		pMat->refCount = 1;
		MFMaterial_Release(pMat);

		pI = MFResource_EnumerateNext(pI, MFRT_Material);
	}

	MFMaterial_UnregisterMaterialType("Standard");
	MFMaterial_UnregisterMaterialType("Effect");

	gMaterialDefList.Deinit();
	gMaterialRegistry.Deinit();
}

void MFMaterial_Update()
{
	MFCALLSTACK;

	MFResourceIterator *pI = MFResource_EnumerateFirst(MFRT_Material);
	while(pI)
	{
		MFMaterial *pMat = (MFMaterial*)MFResource_Get(pI);
		if(pMat->pType->materialCallbacks.pUpdate)
			pMat->pType->materialCallbacks.pUpdate(pMat);

		pI = MFResource_EnumerateNext(pI, MFRT_Material);
	}
}

// interface functions
MF_API int MFMaterial_AddDefinitionsFile(const char *pName, const char *pFilename)
{
	MFCALLSTACK;

	MaterialDefinition *pDef = gMaterialDefList.Create();

	pDef->pName = pName;
	pDef->pIni = MFIni::Create(pFilename);
	if (!pDef->pIni)
	{
		gMaterialDefList.Destroy(pDef);
		MFDebug_Warn(2, "Couldnt create material definitions...");
		return 1;
	}

	pDef->ownsIni = true;

	pDef->pNextDefinition = pDefinitionRegistry;
	pDefinitionRegistry = pDef;

	return 0;
}

MF_API int MFMaterial_AddDefinitionsIni(const char *pName, MFIni *pMatDefs)
{
	MFCALLSTACK;

	MaterialDefinition *pDef = gMaterialDefList.Create();

	pDef->pName = pName;
	pDef->pIni = pMatDefs;
	pDef->ownsIni = false;

	pDef->pNextDefinition = pDefinitionRegistry;
	pDefinitionRegistry = pDef;

	return 0;
}

void MFMaterial_DestroyDefinition(MaterialDefinition *pDefinition)
{
	MFCALLSTACK;

	if(pDefinition->ownsIni)
	{
		MFIni::Destroy(pDefinition->pIni);
		pDefinition->pIni = NULL;
	}

	gMaterialDefList.Destroy(pDefinition);
}

MF_API void MFMaterial_RemoveDefinitions(const char *pName)
{
	MFCALLSTACK;

	MaterialDefinition *pDef = pDefinitionRegistry;

	if(!pDef)
		return;

	if(!MFString_Compare(pDef->pName, pName))
	{
		pDefinitionRegistry = pDef->pNextDefinition;
		MFMaterial_DestroyDefinition(pDef);
		return;
	}

	while(pDef->pNextDefinition)
	{
		if(!MFString_Compare(pDef->pNextDefinition->pName, pName))
		{
			MaterialDefinition *pDestroy = pDef->pNextDefinition;
			pDef->pNextDefinition = pDef->pNextDefinition->pNextDefinition;

			MFMaterial_DestroyDefinition(pDestroy);
			return;
		}

		pDef = pDef->pNextDefinition;
	}
}

MF_API void MFMaterial_RegisterMaterialType(const char *pName, const MFMaterialCallbacks *pCallbacks, size_t instanceDataSize)
{
	MFCALLSTACK;

	MFMaterialType *pMatType;
	pMatType = (MFMaterialType*)MFHeap_Alloc(sizeof(MFMaterialType) + MFString_Length(pName) + 1);

	pMatType->pTypeName = (char*)&pMatType[1];
	MFString_Copy(pMatType->pTypeName, pName);

	MFDebug_Assert(pCallbacks->pBegin, "Material must supply Begin() callback.");

	MFCopyMemory(&pMatType->materialCallbacks, pCallbacks, sizeof(MFMaterialCallbacks));

	pMatType->instanceDataSize = instanceDataSize;

	gMaterialRegistry.Create(pMatType);

	pCallbacks->pRegisterMaterial(pMatType);
}

MF_API void MFMaterial_UnregisterMaterialType(const char *pName)
{
	MFCALLSTACK;

	MFMaterialType *pMatType = MaterialInternal_GetMaterialType(pName);

	MFDebug_Assert(pMatType, MFStr("Material type '%s' doesn't exist!", pName));

	if(pMatType)
	{
		pMatType->materialCallbacks.pUnregisterMaterial();
		gMaterialRegistry.Destroy(pMatType);
		MFHeap_Free(pMatType);
	}
}

MFMaterialType *MaterialInternal_GetMaterialType(const char *pTypeName)
{
	MFCALLSTACK;

	MFMaterialType **ppIterator = gMaterialRegistry.Begin();

	while(*ppIterator)
	{
		if(!MFString_CaseCmp(pTypeName, (*ppIterator)->pTypeName)) return *ppIterator;

		ppIterator++;
	}

	return NULL;
}

MF_API MFMaterial* MFMaterial_Create(const char *pName)
{
	MFCALLSTACK;

	MFMaterial *pMat = MFMaterial_Find(pName);

	if(!pMat)
	{
		pMat = (MFMaterial*)MFHeap_AllocAndZero(sizeof(MFMaterial) + MFString_Length(pName) + 1);

		pName = MFString_Copy((char*)&pMat[1], pName);

		MFResource_AddResource(pMat, MFRT_Material, MFUtil_HashString(pName) ^ 0x0a7e01a1, pName);

		// TODO: how to determine size?
		pMat->pMaterialState = MFStateBlock_Create(256);
		pMat->bStateDirty = true;

		MaterialDefinition *pDef = pDefinitionRegistry;
		while(pDef)
		{
			MFIniLine *pLine = pDef->pIni->GetFirstLine()->FindEntry("section",pName);
			if (pLine)
			{
				MaterialInternal_InitialiseFromDefinition(pDef->pIni, pMat, pName);
				break;
			}

			pDef = pDef->pNextDefinition;
		}

		if(!pDef)
		{
			// assign material type
			pMat->pType = MaterialInternal_GetMaterialType("Standard");

			pMat->pInstanceData = MFHeap_AllocAndZero(pMat->pType->instanceDataSize);
			pMat->pType->materialCallbacks.pCreateInstance(pMat);

			// set diffuse map parameter
			MFMaterial_SetParameterS(pMat, MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap, pName);
		}
	}

	return pMat;
}

MF_API int MFMaterial_Release(MFMaterial *pMaterial)
{
	return MFResource_Release(pMaterial);
}

MF_API MFMaterial* MFMaterial_Find(const char *pName)
{
	return (MFMaterial*)MFResource_Find(MFUtil_HashString(pName) ^ 0x0a7e01a1);
}

MF_API MFMaterial* MFMaterial_GetCurrent()
{
	return pCurrentMaterial;
}

MF_API const char *MFMaterial_GetMaterialName(const MFMaterial *pMaterial)
{
	return pMaterial->pName;
}

MF_API MFStateBlock* MFMaterial_GetMaterialStateBlock(MFMaterial *pMaterial)
{
	if(pMaterial->bStateDirty)
	{
		pMaterial->pType->materialCallbacks.pBuildStateBlock(pMaterial);
		pMaterial->bStateDirty = false;
	}

	return pMaterial->pMaterialState;
}

MF_API void MFMaterial_SetMaterial(const MFMaterial *pMaterial)
{
	MFCALLSTACK;

	if(!pMaterial)
		pMaterial = MFMaterial_GetStockMaterial(MFMat_White);

	pCurrentMaterial = (MFMaterial*)pMaterial;
}

MF_API MFMaterial* MFMaterial_GetStockMaterial(MFStockMaterials materialIdentifier)
{
	MFCALLSTACK;

	switch(materialIdentifier)
	{
		case MFMat_White:
			return pWhiteMaterial;
		case MFMat_Unavailable:
			return pNoneMaterial;
		case MFMat_SysLogoSmall:
			return pSysLogoSmall;
		case MFMat_SysLogoLarge:
			return pSysLogoLarge;
		case MFMat_USB:
			return pUSB;
		case MFMat_Connected:
			return pConnected;
		case MFMat_Disconnected:
			return pDisconnected;
		case MFMat_Power:
			return pPower;
		case MFMat_Charging:
			return pCharging;
		default:
			MFDebug_Assert(false, "Invalid Stock Material");
	}

	return NULL;
}


// internal functions
const char* MaterialInternal_GetIDString()
{
	MFCALLSTACK;
/*
	char *id = &stringBuffer[stringBufferOffset];
	*id = NULL;

	for(int a=0; a<32; a++)
	{
		if((flags>>a)&1) MFString_Cat(id, matDesc[a]);
	}

	stringBufferOffset += MFString_Length(id) + 1;
*/
	return NULL;
}

void MaterialInternal_InitialiseFromDefinition(MFIni *pDefIni, MFMaterial *pMat, const char *pDefinition)
{
	MFCALLSTACK;

	MFIniLine *pLine = pDefIni->GetFirstLine()->FindEntry("section", pDefinition);

	if(pLine)
	{
		pLine = pLine->Sub();

		if(pLine && pLine->IsString(0, "type"))
		{
			pMat->pType = MaterialInternal_GetMaterialType(pLine->GetString(1));

			if(!pMat->pType)
				pMat->pType = MaterialInternal_GetMaterialType("Standard");

			pLine = pLine->Next();
		}
		else
		{
			pMat->pType = MaterialInternal_GetMaterialType("Standard");
		}

		pMat->pInstanceData = MFHeap_AllocAndZero(pMat->pType->instanceDataSize);

		if(pMat->pType->materialCallbacks.pCreateInstance)
			pMat->pType->materialCallbacks.pCreateInstance(pMat);

		while(pLine)
		{
			if(pLine->IsString(0,"type"))
			{
				MFDebug_Warn(2, MFStr("'type' MUST be the first parameter in a material definition... Ignored, Using type '%s'.", pMat->pType->pTypeName));
			}
			else if(pLine->IsString(0,"alias"))
			{
				MFDebug_Warn(2, "'alias' MUST be the first parameter in a material definition... Ignored.");

MFDebug_Assert(false, "Fix Me!!!");
//				const char *pAlias = pLine->GetString(1);
//				MaterialInternal_InitialiseFromDefinition(pDefIni, pMat, pAlias);
			}
			else
			{
				const char *pParam = pLine->GetString(0);
				const MFMaterialParameterInfo *pInfo = MFMaterial_GetParameterInfoFromName(pMat, pParam);

				int lineArg = 1;
				int param = pInfo->parameterIndex;
				int argIndex = 0;

				switch(pInfo->argIndex.type)
				{
					case MFParamType_Int:
						argIndex = pLine->GetInt(lineArg++);
						break;
					case MFParamType_Enum:
						argIndex = pLine->GetEnum(lineArg++, pInfo->argIndex.pEnumKeys);
						break;
					default:
						argIndex = pInfo->argIndex.defaultValue;
						break;
				}

				switch(pInfo->argIndexHigh.type)
				{
					case MFParamType_Int:
						argIndex |= pLine->GetInt(lineArg++) << 16;
						break;
					case MFParamType_Enum:
						argIndex |= pLine->GetEnum(lineArg++, pInfo->argIndexHigh.pEnumKeys) << 16;
						break;
					default:
						argIndex |= pInfo->argIndexHigh.defaultValue << 16;
						break;
				}

				if(pInfo->numValues == 1)
				{
					switch(pInfo->pValues[0].type)
					{
						case MFParamType_Constant:
						{
							MFMaterial_SetParameterI(pMat, param, argIndex, pInfo->pValues[0].defaultValue);
							break;
						}

						case MFParamType_String:
						{
							const char *pString = pLine->GetString(lineArg);
							MFMaterial_SetParameterS(pMat, param, argIndex, pString);
							break;
						}

						case MFParamType_Float:
						{
							float value = pLine->GetFloat(lineArg);
							MFMaterial_SetParameterF(pMat, param, argIndex, value);
							break;
						}

						case MFParamType_Int:
						{
							int value = pLine->GetStringCount() > lineArg ? pLine->GetInt(lineArg) : pInfo->pValues[0].defaultValue;
							MFMaterial_SetParameterI(pMat, param, argIndex, value);
							break;
						}

						case MFParamType_Enum:
						{
							int value;
							if(pLine->GetStringCount() > lineArg)
								value = pLine->GetEnum(lineArg, pInfo->pValues[0].pEnumKeys);
							else
								value = pInfo->pValues[0].defaultValue;
							MFMaterial_SetParameterI(pMat, param, argIndex, value);
							break;
						}

						case MFParamType_Bool:
						{
							bool value = pLine->GetStringCount() > lineArg ? pLine->GetBool(lineArg) : !!pInfo->pValues[0].defaultValue;
							MFMaterial_SetParameterI(pMat, param, argIndex, value ? 1 : 0);
							break;
						}

						case MFParamType_Colour:
						{
							MFVector vector = pLine->GetColour(lineArg);
							MFMaterial_SetParameterV(pMat, param, argIndex, vector);
							break;
						}

						case MFParamType_Vector3:
						{
							MFVector vector = pLine->GetVector3(lineArg);
							MFMaterial_SetParameterV(pMat, param, argIndex, vector);
							break;
						}

						case MFParamType_Vector4:
						{
							MFVector vector = pLine->GetVector4(lineArg);
							MFMaterial_SetParameterV(pMat, param, argIndex, vector);
							break;
						}

						case MFParamType_Matrix:
						{
							MFDebug_Assert(false, "Cant read a matrix from an ini file... yet...");
							break;
						}

						default:
							MFDebug_Assert(false, "Unknown parameter type..");
					}
				}
				else if(pInfo->numValues > 1)
				{
					// produce a struct representing the args
					MFALIGN_BEGIN(16)
						char argBuffer[256]
					MFALIGN_END(16);
					int offset = 0;

					for(int a=0; a<pInfo->numValues; a++)
					{
						switch(pInfo->pValues[a].type)
						{
							case MFParamType_Constant:
							{
								offset = MFALIGN(offset, sizeof(int));
								(int&)argBuffer[offset] = pInfo->pValues[a].defaultValue;
								offset += sizeof(int);
								break;
							}

							case MFParamType_String:
							{
								offset = MFALIGN(offset, sizeof(const char *));
								(const char *&)argBuffer[offset] = pLine->GetString(lineArg++);
								offset += sizeof(const char *);
								break;
							}

							case MFParamType_Float:
							{
								offset = MFALIGN(offset, sizeof(float));
								(float&)argBuffer[offset] = pLine->GetFloat(lineArg++);
								offset += sizeof(float);
								break;
							}

							case MFParamType_Int:
							{
								offset = MFALIGN(offset, sizeof(int));
								(int&)argBuffer[offset] = pLine->GetInt(lineArg++);
								offset += sizeof(int);
								break;
							}

							case MFParamType_Enum:
							{
								offset = MFALIGN(offset, sizeof(int));
								(int&)argBuffer[offset] = pLine->GetEnum(lineArg++, pInfo->pValues[a].pEnumKeys);
								offset += sizeof(int);
								break;
							}

							case MFParamType_Bool:
							{
								offset = MFALIGN(offset, sizeof(bool));
								(bool&)argBuffer[offset] = pLine->GetBool(lineArg++);
								offset += sizeof(bool);
								break;
							}

							case MFParamType_Colour:
							case MFParamType_Vector3:
							case MFParamType_Vector4:
							case MFParamType_Matrix:
							{
								MFDebug_Assert(false, "Cant read type into structure... yet...");
								break;
							}

							default:
								MFDebug_Assert(false, "Unknown parameter type..");
						}
					}

					MFMaterial_SetParameter(pMat, param, argIndex, (uintp)argBuffer);
				}
			}

			pLine = pLine->Next();
		}
	}
}

MF_API int MFMaterial_GetNumParameters(const MFMaterial *pMaterial)
{
	if(!pMaterial->pType->materialCallbacks.pGetNumParams)
		return 0;
	return pMaterial->pType->materialCallbacks.pGetNumParams();
}

MF_API const char* MFMaterial_GetParameterName(const MFMaterial *pMaterial, int parameterIndex)
{
	MFDebug_Assert(pMaterial->pType->materialCallbacks.pGetParameterInfo, "Material does not supply a GetParameterInfo() function.");

	const MFMaterialParameterInfo *pInfo = pMaterial->pType->materialCallbacks.pGetParameterInfo(parameterIndex);
	return pInfo->pParameterName;
}

MF_API int MFMaterial_GetParameterIndexFromName(const MFMaterial *pMaterial, const char *pParameterName)
{
	MFDebug_Assert(pMaterial->pType->materialCallbacks.pGetParameterInfo, "Material does not supply a GetParameterInfo() function.");

	int numParams = MFMaterial_GetNumParameters(pMaterial);
	for(int a=0; a<numParams; a++)
	{
		const MFMaterialParameterInfo *pInfo = pMaterial->pType->materialCallbacks.pGetParameterInfo(a);
		if(!MFString_CaseCmp(pInfo->pParameterName, pParameterName))
			return a;
	}

	return -1;
}

MF_API const MFMaterialParameterInfo *MFMaterial_GetParameterInfo(const MFMaterial *pMaterial, int parameterIndex)
{
	MFDebug_Assert(pMaterial->pType->materialCallbacks.pGetParameterInfo, "Material does not supply a GetParameterInfo() function.");
	return pMaterial->pType->materialCallbacks.pGetParameterInfo(parameterIndex);
}

MF_API const MFMaterialParameterInfo *MFMaterial_GetParameterInfoFromName(const MFMaterial *pMaterial, const char *pParameterName)
{
	int param = MFMaterial_GetParameterIndexFromName(pMaterial, pParameterName);
	return MFMaterial_GetParameterInfo(pMaterial, param);
}

MF_API void MFMaterial_SetParameter(MFMaterial *pMaterial, int parameterIndex, int argIndex, uintp value)
{
	MFDebug_Assert(pMaterial->pType->materialCallbacks.pSetParameter, "Material does not supply a SetParameter() function.");
	pMaterial->pType->materialCallbacks.pSetParameter(pMaterial, parameterIndex, argIndex, value);

	// dirty the stateblock
	// TODO: this is pretty harsh, perhaps we can reduce the damage?
	pMaterial->bStateDirty = true;
}

MF_API uintp MFMaterial_GetParameter(const MFMaterial *pMaterial, int parameterIndex, int argIndex, void *pValue)
{
	MFDebug_Assert(pMaterial->pType->materialCallbacks.pGetParameter, "Material does not supply a GetParameter() function.");
	return pMaterial->pType->materialCallbacks.pGetParameter(pMaterial, parameterIndex, argIndex, pValue);
}


// material browser
MaterialBrowser::MaterialBrowser()
{
	selection = 0;
	type = MenuType_TextureBrowser;
}

void MaterialBrowser::Draw()
{

}

void MaterialBrowser::Update()
{
	if(MFInput_WasPressed(Button_XB_Y, IDD_Gamepad))
		pCurrentMenu = pParent;
}

#define TEX_SIZE 64.0f
float MaterialBrowser::ListDraw(bool selected, const MFVector &_pos, float maxWidth)
{
	MFVector pos = _pos;

	MFResourceIterator *pI = MFResource_EnumerateFirst(MFRT_Material);
	for(int a=0; a<selection; a++)
		pI = MFResource_EnumerateNext(pI, MFRT_Material);

	MFMaterial *pMaterial = (MFMaterial*)MFResource_Get(pI);

	MFFont_DrawText(MFFont_GetDebugFont(), pos+MakeVector(0.0f, ((TEX_SIZE+8.0f)*0.5f)-(MENU_FONT_HEIGHT*0.5f)-MENU_FONT_HEIGHT, 0.0f), MENU_FONT_HEIGHT, selected ? MakeVector(1,1,0,1) : MFVector::one, MFStr("%s:", name));
	MFFont_DrawText(MFFont_GetDebugFont(), pos+MakeVector(10.0f, ((TEX_SIZE+8.0f)*0.5f)-(MENU_FONT_HEIGHT*0.5f), 0.0f), MENU_FONT_HEIGHT, selected ? MakeVector(1,1,0,1) : MFVector::one, MFStr("%s", pMaterial->pName));
	MFFont_DrawText(MFFont_GetDebugFont(), pos+MakeVector(10.0f, ((TEX_SIZE+8.0f)*0.5f)-(MENU_FONT_HEIGHT*0.5f)+MENU_FONT_HEIGHT, 0.0f), MENU_FONT_HEIGHT, selected ? MakeVector(1,1,0,1) : MFVector::one, MFStr("Type: %s Refs: %d", pMaterial->pType->pTypeName, pMaterial->refCount));

	pos += MakeVector(maxWidth - (TEX_SIZE + 4.0f + 5.0f), 2.0f, 0.0f);

	MFPrimitive(PT_TriStrip|PT_Untextured);

	MFBegin(4);
	MFSetColourV(MFVector::white);
	MFSetPositionV(pos);
	MFSetPositionV(pos + MakeVector(TEX_SIZE + 4.0f, 0.0f, 0.0f));
	MFSetPositionV(pos + MakeVector(0.0f, TEX_SIZE + 4.0f, 0.0f));
	MFSetPositionV(pos + MakeVector(TEX_SIZE + 4.0f, TEX_SIZE + 4.0f, 0.0f));
	MFEnd();

	pos += MakeVector(2.0f, 2.0f, 0.0f);

	const int numSquares = 7;
	for(int a=0; a<numSquares; a++)
	{
		for(int b=0; b<numSquares; b++)
		{
			float x, y, w, h;
			w = TEX_SIZE/(float)numSquares;
			h = TEX_SIZE/(float)numSquares;
			x = pos.x + (float)b*w;
			y = pos.y + (float)a*h;

			MFBegin(4);
			MFSetColourV(((a+b)&1) ? MakeVector(.75f, .75f, .75f, 1.f) : MakeVector(.2f, .2f, .2f, 1.f));
			MFSetPosition(x,y,0);
			MFSetPosition(x+w,y,0);
			MFSetPosition(x,y+h,0);
			MFSetPosition(x+w,y+h,0);
			MFEnd();
		}
	}

	MFMaterial_SetMaterial(pMaterial);

	MFPrimitive(PT_TriStrip);

	MFBegin(4);
	MFSetColourV(MFVector::white);
	MFSetTexCoord1(0.0f,0.0f);
	MFSetPositionV(pos);
	MFSetTexCoord1(1.0f,0.0f);
	MFSetPositionV(pos + MakeVector(TEX_SIZE, 0.0f, 0.0f));
	MFSetTexCoord1(0.0f,1.0f);
	MFSetPositionV(pos + MakeVector(0.0f, TEX_SIZE, 0.0f));
	MFSetTexCoord1(1.0f,1.0f);
	MFSetPositionV(pos + MakeVector(TEX_SIZE, TEX_SIZE, 0.0f));
	MFEnd();

	return TEX_SIZE + 8.0f;
}

void MaterialBrowser::ListUpdate(bool selected)
{
	if(selected)
	{
		int matCount = MFResource_GetNumResources(MFRT_Material);

		if(MFInput_WasPressed(Button_DLeft, IDD_Gamepad))
		{
			selection = selection <= 0 ? matCount-1 : selection-1;

			if(pCallback)
				pCallback(this, pUserData);
		}
		else if(MFInput_WasPressed(Button_DRight, IDD_Gamepad))
		{
			selection = selection >= matCount-1 ? 0 : selection+1;

			if(pCallback)
				pCallback(this, pUserData);
		}
	}
}

MFVector MaterialBrowser::GetDimensions(float maxWidth)
{
	return MakeVector(maxWidth, TEX_SIZE + 8.0f, 0.0f);
}
