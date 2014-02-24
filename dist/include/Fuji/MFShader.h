/**
 * @file MFShader.h
 * @brief Shader management.
 * @author Manu Evans
 * @defgroup MFShader Shaders
 * @{
 */

#pragma once
#if !defined(_MFSHADER_H)
#define _MFSHADER_H

struct MFShader;

typedef void (MFShader_ConfigureCallback)();
typedef void (MFShader_ExecuteCallback)(void *pWorkload);

enum MFShaderLanguage
{
	MFSL_Unknown = -1,

	MFSL_HLSL = 0,
	MFSL_GLSL,
	MFSL_Cg,
	MFSL_VSAsm,
	MFSL_PSAsm,

	MFSL_Max,
	MFSL_ForceInt = 0x7FFFFFFF
};

enum MFShaderType
{
	MFST_VertexShader,
	MFST_PixelShader,		// GLSL: Fragment shader
	MFST_GeometryShader,
	MFST_DomainShader,		// GLSL: Evaluation shader
	MFST_HullShader,		// GLSL: Control shader
	MFST_ComputeShader,

	MFST_Max,

	MFST_FragmentShader = MFST_PixelShader,
	MFST_EvaluationShader = MFST_DomainShader,
	MFST_ControlShader = MFST_HullShader,

	MFST_ForceInt = 0x7FFFFFFF
};

enum MFShaderInputType
{
	MFShader_IT_Unknown = -1,

	MFShader_IT_Vector = 0,
	MFShader_IT_Matrix,
	MFShader_IT_Float,
	MFShader_IT_Int,
	MFShader_IT_Bool,
	MFShader_IT_Sampler,

	MFShader_IT_Max,
	MFShader_IT_ForceInt = 0x7FFFFFFF
};

struct MFShaderMacro
{
	const char *pDefine;
	const char *pValue;
};

struct MFShaderInput
{
	const char *pName;
	MFShaderInputType type;
	uint32 numRows			: 3;
	uint32 columnMajor		: 1;
	uint32 numElements		: 12;
	uint32 constantRegister	: 8;
	uint32 numRegisters		: 8;
};

MF_API MFShader* MFShader_CreateFromFile(MFShaderType type, const char *pFilename, MFShaderMacro *pMacros = NULL, MFShaderLanguage language = MFSL_Unknown);
MF_API MFShader* MFShader_CreateFromString(MFShaderType type, const char *pShader, MFShaderMacro *pMacros = NULL, const char *pName = NULL, const char *pFilename = NULL, int startingLine = 0, MFShaderLanguage language = MFSL_Unknown);
MF_API MFShader* MFShader_CreateFromCallbacks(MFShaderType type, MFShader_ConfigureCallback *pConfigureFunc, MFShader_ExecuteCallback *pExecuteFunc, const char *pName = NULL);
MF_API int MFShader_Release(MFShader *pShader);

#endif // _MFSHADER_H

/** @} */
