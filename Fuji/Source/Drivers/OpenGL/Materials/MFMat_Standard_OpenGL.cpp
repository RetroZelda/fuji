#include "Fuji.h"

#if MF_RENDERER == MF_DRIVER_OPENGL || defined(MF_RENDERPLUGIN_OPENGL)

#if defined(MF_RENDERPLUGIN_OPENGL)
	#define MFMat_Standard_RegisterMaterial MFMat_Standard_RegisterMaterial_OpenGL
	#define MFMat_Standard_UnregisterMaterial MFMat_Standard_UnregisterMaterial_OpenGL
	#define MFMat_Standard_Begin MFMat_Standard_Begin_OpenGL
	#define MFMat_Standard_CreateInstancePlatformSpecific MFMat_Standard_CreateInstancePlatformSpecific_OpenGL
	#define MFMat_Standard_DestroyInstancePlatformSpecific MFMat_Standard_DestroyInstancePlatformSpecific_OpenGL
#endif

#include "MFRenderState_Internal.h"
#include "MFTexture_Internal.h"
#include "MFMaterial_Internal.h"
#include "Materials/MFMat_Standard_Internal.h"

#include "../MFOpenGL.h"


static const GLenum glBlendOp[MFBlendOp_BlendOpCount] =
{
    GL_FUNC_ADD,
    GL_FUNC_SUBTRACT,
    GL_FUNC_REVERSE_SUBTRACT,
    GL_MIN,
    GL_MAX
};

static const GLenum glBlendArg[MFBlendArg_Max] =
{
	GL_ZERO,
	GL_ONE,
	GL_SRC_COLOR,
	GL_ONE_MINUS_SRC_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_COLOR,
	GL_ONE_MINUS_DST_COLOR,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA,
	GL_SRC_ALPHA_SATURATE,
	GL_CONSTANT_COLOR,
	GL_ONE_MINUS_CONSTANT_COLOR,
	GL_SRC1_COLOR,
	GL_ONE_MINUS_SRC1_COLOR,
	GL_SRC1_ALPHA,
	GL_ONE_MINUS_SRC1_ALPHA,
};

static const GLenum glCompareFunc[MFComparisonFunc_Max] =
{
	GL_NEVER,
	GL_LESS,
	GL_EQUAL,
	GL_LEQUAL,
	GL_GREATER,
	GL_NOTEQUAL,
	GL_GEQUAL,
	GL_ALWAYS
};

#if defined(MF_OPENGL_SUPPORT_SHADERS)
	static GLuint gDefVertexShader = 0;
	static GLuint gDefFragmentShaderUntextured = 0;
	static GLuint gDefFragmentShaderTextured = 0;
	static GLuint gDefFragmentShaderMultiTextured = 0;

	static GLuint gDefShaderProgramUntextured = 0;
	static GLuint gDefShaderProgramTextured = 0;
	static GLuint gDefShaderProgramMultiTextured = 0;

	static const GLchar gVertexShader[] = "					\n\
		uniform mat4 wvMatrix;								\n\
		uniform mat4 vpMatrix;								\n\
		uniform mat4 wvpMatrix;								\n\
		uniform mat4 texMatrix;								\n\
															\n\
		attribute vec3 vPos;								\n\
		attribute vec3 vNormal;								\n\
		attribute vec4 vColour;								\n\
		attribute vec2 vUV0;								\n\
		attribute vec2 vUV1;								\n\
															\n\
		varying vec4 oColour;								\n\
		varying vec2 oUV0;									\n\
		varying vec2 oUV1;									\n\
															\n\
		void main()											\n\
		{													\n\
			oColour = vColour;								\n\
			oUV0 = vUV0;									\n\
			oUV1 = vUV1;									\n\
			gl_Position = wvpMatrix * vec4(vPos, 1.0);		\n\
		}													";

	static const GLchar gFragmentShaderUntextured[] = "			\n\
		varying vec4 oColour;									\n\
		void main(void)											\n\
		{														\n\
			gl_FragColor = oColour;								\n\
		}														";

	static const GLchar gFragmentShaderTextured[] = "			\n\
		uniform sampler2D diffuse;								\n\
		varying vec4 oColour;									\n\
		varying vec2 oUV0;										\n\
		void main(void)											\n\
		{														\n\
			gl_FragColor = texture2D(diffuse, oUV0) * oColour;	\n\
		}														";

	static const GLchar gFragmentShaderMultiTextured[] = "		\n\
		uniform sampler2D diffuse;								\n\
		uniform sampler2D detail;								\n\
		varying vec4 oColour;									\n\
		varying vec2 oUV0;										\n\
		varying vec2 oUV1;										\n\
		void main(void)											\n\
		{														\n\
			vec4 image = texture2D(diffuse, oUV0);				\n\
			vec4 colour = texture2D(detail, oUV0) * oColour;	\n\
			gl_FragColor = vec4(image.xyz, 0) + colour;			\n\
		}														";
#endif

int MFMat_Standard_RegisterMaterial(MFMaterialType *pType)
{
	// probably compile the shaders now i guess...
	gDefVertexShader = MFRenderer_OpenGL_CompileShader(gVertexShader, MFOGL_ShaderType_VertexShader);
	gDefFragmentShaderUntextured = MFRenderer_OpenGL_CompileShader(gFragmentShaderUntextured, MFOGL_ShaderType_FragmentShader);
	gDefFragmentShaderTextured = MFRenderer_OpenGL_CompileShader(gFragmentShaderTextured, MFOGL_ShaderType_FragmentShader);
	gDefFragmentShaderMultiTextured = MFRenderer_OpenGL_CompileShader(gFragmentShaderMultiTextured, MFOGL_ShaderType_FragmentShader);

#if defined(MF_OPENGL_ES)
	glReleaseShaderCompiler();
#endif

	// create and link a program
	gDefShaderProgramUntextured = MFRenderer_OpenGL_CreateProgram(gDefVertexShader, gDefFragmentShaderUntextured);
	gDefShaderProgramTextured = MFRenderer_OpenGL_CreateProgram(gDefVertexShader, gDefFragmentShaderTextured);
	gDefShaderProgramMultiTextured = MFRenderer_OpenGL_CreateProgram(gDefVertexShader, gDefFragmentShaderMultiTextured);

	return 0;
}

void MFMat_Standard_UnregisterMaterial()
{
	glDeleteProgram(gDefShaderProgramUntextured);
	glDeleteProgram(gDefShaderProgramTextured);
	glDeleteProgram(gDefShaderProgramMultiTextured);

	glDeleteShader(gDefVertexShader);
	glDeleteShader(gDefFragmentShaderUntextured);
	glDeleteShader(gDefFragmentShaderTextured);
	glDeleteShader(gDefFragmentShaderMultiTextured);
}

inline void MFMat_Standard_SetSamplerState(int texture, MFSamplerState *pSampler, const char *pName)
{
	MFRenderer_OpenGL_SetUniformS(pName, texture);
	GLint sampler = (GLint)(size_t)pSampler->pPlatformData;
	glBindSampler(texture, sampler);
}

int MFMat_Standard_Begin(MFMaterial *pMaterial, MFRendererState &state)
{
	MFCALLSTACK;

#if MFMatStandard_TexFilter_Max > 4
	#error "glTexFilters only supports 4 mip filters..."
#endif

	bool bDetailPresent = state.isSet(MFSB_CT_Bool, MFSCB_DetailMapSet);
	bool bDiffusePresent = state.isSet(MFSB_CT_Bool, MFSCB_DiffuseSet);

	if(bDetailPresent)
	{
		// set detail map
		MFTexture *pDetail = state.pTextures[MFSCT_DetailMap];
		if(state.pTexturesSet[MFSCT_DetailMap] != pDetail)
		{
			state.pTexturesSet[MFSCT_DetailMap] = pDetail;

			glActiveTexture(GL_TEXTURE0 + MFSCT_DetailMap);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)pDetail->pInternalData);
		}

		// set detail map sampler
		MFSamplerState *pDetailSamp = (MFSamplerState*)state.pRenderStates[MFSCRS_DetailMapSamplerState];
		if(state.pRenderStatesSet[MFSCRS_DetailMapSamplerState] != pDetailSamp)
		{
			state.pRenderStatesSet[MFSCRS_DetailMapSamplerState] = pDetailSamp;

			MFRenderer_OpenGL_SetShaderProgram(gDefShaderProgramMultiTextured);
			MFMat_Standard_SetSamplerState(MFSCT_DetailMap, pDetailSamp, "detail");
		}
	}
	else
	{
		if(state.pTexturesSet[MFSCT_DetailMap] != NULL)
		{
			state.pTexturesSet[MFSCT_DetailMap] = NULL;

			glActiveTexture(GL_TEXTURE0 + MFSCT_DetailMap);
			glDisable(GL_TEXTURE_2D);
		}
	}

	if(bDiffusePresent)
	{
		// set diffuse map
		MFTexture *pDiffuse = state.pTextures[MFSCT_Diffuse];
		if(state.pTexturesSet[MFSCT_Diffuse] != pDiffuse || state.boolChanged(MFSCB_DetailMapSet))
		{
			state.pTexturesSet[MFSCT_Diffuse] = pDiffuse;

			glActiveTexture(GL_TEXTURE0 + MFSCT_Diffuse);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)pDiffuse->pInternalData);
		}

		// set diffuse map sampler
		MFSamplerState *pDiffuseSamp = (MFSamplerState*)state.pRenderStates[MFSCRS_DiffuseSamplerState];
		if(state.pRenderStatesSet[MFSCRS_DiffuseSamplerState] != pDiffuseSamp || state.boolChanged(MFSCB_DetailMapSet))
		{
			state.pRenderStatesSet[MFSCRS_DiffuseSamplerState] = pDiffuseSamp;
			MFRenderer_OpenGL_SetShaderProgram(gDefShaderProgramTextured);
			MFMat_Standard_SetSamplerState(MFSCT_Diffuse, pDiffuseSamp, "diffuse");
		}
	}
	else
	{
		if(state.pTexturesSet[MFSCT_Diffuse] != NULL)
		{
			state.pTexturesSet[MFSCT_Diffuse] = NULL;

			glActiveTexture(GL_TEXTURE0 + MFSCT_Diffuse);
			glDisable(GL_TEXTURE_2D);

			MFRenderer_OpenGL_SetShaderProgram(gDefShaderProgramUntextured);
		}
	}

	if(state.pMatrixStatesSet[MFSCM_Projection] != state.pMatrixStates[MFSCM_Projection])
	{
		MFMatrix *pProj = state.pMatrixStates[MFSCM_Projection];
		state.pMatrixStatesSet[MFSCM_Projection] = pProj;

		MFRenderer_OpenGL_SetMatrix(MFOGL_MatrixType_Projection, *pProj);
	}

	if(state.pMatrixStatesSet[MFSCM_WorldView] != state.pMatrixStates[MFSCM_WorldView])
	{
		MFMatrix *pWV = state.getDerivedMatrix(MFSCM_WorldView);
		state.pMatrixStates[MFSCM_WorldView] = pWV;

		MFRenderer_OpenGL_SetMatrix(MFOGL_MatrixType_WorldView, *pWV);
	}

	if(state.pMatrixStatesSet[MFSCM_UV0] != state.pMatrixStates[MFSCM_UV0])
	{
		MFMatrix *pUV0 = state.pMatrixStates[MFSCM_UV0];
		state.pMatrixStatesSet[MFSCM_UV0] = pUV0;

		MFRenderer_OpenGL_SetMatrix(MFOGL_MatrixType_Texture, *pUV0);
	}
/*
	if(state.pVectorStatesSet[MFSCV_MaterialDiffuseColour] != state.pVectorStates[MFSCV_MaterialDiffuseColour])
	{
		MFVector *pDiffuseColour = state.pVectorStates[MFSCV_MaterialDiffuseColour];
		state.pVectorStatesSet[MFSCV_MaterialDiffuseColour] = pDiffuseColour;

//		pd3dDevice->SetVertexShaderConstantF(r_modelColour, (float*)pDiffuseColour, 1);
	}
*/

	// blend state
	MFBlendState *pBlendState = (MFBlendState*)state.pRenderStates[MFSCRS_BlendState];
	if(state.pRenderStatesSet[MFSCRS_BlendState] != pBlendState)
	{
		state.pRenderStatesSet[MFSCRS_BlendState] = pBlendState;

		if(pBlendState->stateDesc.bIndependentBlendEnable)
		{
			for(int i=0; i<8; ++i)
			{
				MFBlendStateDesc::RenderTargetBlendDesc &target = pBlendState->stateDesc.renderTarget[i];
				if(target.bEnable)
				{
					glEnable(GL_BLEND);
					glBlendEquationSeparatei(i, glBlendOp[target.blendOp], glBlendOp[target.blendOpAlpha]);
					glBlendFuncSeparatei(i, glBlendArg[target.srcBlend], glBlendArg[target.destBlend], glBlendArg[target.srcBlendAlpha], glBlendArg[target.destBlendAlpha]);
				}
				else
					glDisable(GL_BLEND);
				glColorMaski(i, target.writeMask & MFColourWriteEnable_Red, target.writeMask & MFColourWriteEnable_Green, target.writeMask & MFColourWriteEnable_Blue, target.writeMask & MFColourWriteEnable_Alpha);
			}
		}
		else
		{
			MFBlendStateDesc::RenderTargetBlendDesc &target = pBlendState->stateDesc.renderTarget[0];
			if(target.bEnable)
			{
				glEnable(GL_BLEND);
				glBlendEquationSeparate(glBlendOp[target.blendOp], glBlendOp[target.blendOpAlpha]);
				glBlendFuncSeparate(glBlendArg[target.srcBlend], glBlendArg[target.destBlend], glBlendArg[target.srcBlendAlpha], glBlendArg[target.destBlendAlpha]);
			}
			else
				glDisable(GL_BLEND);
			glColorMask(target.writeMask & MFColourWriteEnable_Red, target.writeMask & MFColourWriteEnable_Green, target.writeMask & MFColourWriteEnable_Blue, target.writeMask & MFColourWriteEnable_Alpha);
		}
	}

	// rasteriser state
	MFRasteriserState *pRasteriserState = (MFRasteriserState*)state.pRenderStates[MFSCRS_RasteriserState];
	if(state.pRenderStatesSet[MFSCRS_RasteriserState] != pRasteriserState)
	{
		state.pRenderStatesSet[MFSCRS_RasteriserState] = pRasteriserState;

		switch(pRasteriserState->stateDesc.cullMode)
		{
			case MFCullMode_None:
				glDisable(GL_CULL_FACE);
				break;
			case MFCullMode_CCW:
				glEnable(GL_CULL_FACE);
				glFrontFace(GL_CW);
				glCullFace(GL_BACK);
				break;
			case MFCullMode_CW:
				glEnable(GL_CULL_FACE);
				glFrontFace(GL_CCW);
				glCullFace(GL_BACK);
				break;
			default:
				MFUNREACHABLE;
		}
	}

	// depth/stencil state
	MFDepthStencilState *pDSState = (MFDepthStencilState*)state.pRenderStates[MFSCRS_DepthStencilState];
	if(state.pRenderStatesSet[MFSCRS_DepthStencilState] != pDSState)
	{
		state.pRenderStatesSet[MFSCRS_DepthStencilState] = pDSState;

		if(pDSState->stateDesc.bDepthEnable)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(glCompareFunc[pDSState->stateDesc.depthFunc]);
			glDepthMask(pDSState->stateDesc.depthWriteMask == MFDepthWriteMask_Zero ? GL_FALSE : GL_TRUE);
		}
		else
			glDisable(GL_DEPTH_TEST);
	}

	// setup alpha test
	if(state.boolChanged(MFSCB_AlphaTest) || (state.pVectorStatesSet[MFSCV_RenderState] != state.pVectorStates[MFSCV_RenderState] && state.getBool(MFSCB_AlphaTest)))
	{
		MFVector *pRS = state.pVectorStates[MFSCV_RenderState];
		state.pVectorStatesSet[MFSCV_RenderState] = pRS;
		state.boolsSet = (state.boolsSet & ~MFBIT(MFSCB_AlphaTest)) | (state.bools & MFBIT(MFSCB_AlphaTest));

#if !defined(MF_OPENGL_ES)
		if(state.getBool(MFSCB_AlphaTest))
		{
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GEQUAL, pRS->x);
		}
		else
			glDisable(GL_ALPHA_TEST);
#else
		// TODO: do something here...
		//I guess we need to implement the alpha test in the shader...
#endif
	}
/*
	// set clour/alpha scales
	if(state.pVectorStatesSet[MFSCV_User0] != state.pVectorStates[MFSCV_User0])
	{
		MFVector *pMask = state.pVectorStates[MFSCV_User0];
		state.pVectorStatesSet[MFSCV_User0] = pMask;

//		pd3dDevice->SetVertexShaderConstantF(r_colourMask, (float*)pMask, 1);
	}
*/

	MFCheckForOpenGLError(true);

	// update the bools 'set' state
	state.boolsSet = state.bools & state.rsSet[MFSB_CT_Bool];

	return 0;
}

void MFMat_Standard_CreateInstancePlatformSpecific(MFMaterial *pMaterial)
{
}

void MFMat_Standard_DestroyInstancePlatformSpecific(MFMaterial *pMaterial)
{
}

#endif
