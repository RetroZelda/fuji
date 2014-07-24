/**
 * @file MFAnimation.h
 * @brief A set of functions for managing mesh animation.
 * @author Manu Evans
 * @defgroup MFAnimation Animation System
 * @{
 */

#if !defined(_MFANIMATION_H)
#define _MFANIMATION_H

struct MFModel;
struct MFMatrix;

/**
 * @struct MFAnimation
 * Represents a Fuji animation.
 */
struct MFAnimation;

/**
 * Create an animation from the filesystem.
 * Creates an animation from the filesystem.
 * @param pFilename Filename of the animation to load.
 * @param pModel MFModel instance the animation will be bound to.
 * @return A new instance of the specified animation.
 * @see MFAnimation_Release(), MFAnimation_CalculateMatrices()
 */
MF_API MFAnimation* MFAnimation_Create(const char *pFilename, MFModel *pModel);

/**
 * Destroy an animation.
 * Destroys an animation instance.
 * @param pAnimation Animation instance to be destroyed.
 * @return The new reference count of the animation. If the returned reference count is 0, the animation is destroyed.
 * @see MFAnimation_Create()
 */
MF_API int MFAnimation_Release(MFAnimation *pAnimation);

/**
 * Calculate the animation matrices.
 * Calculates the animation matrices for the current frame.
 * @param pAnimation Animation instance.
 * @param pLocalToWorld Optional pointer to a LocalToWorld matrix that will be multiplied into the animation matrices.
 * @return A pointer to the array of calculated animation matrices.
 * @see MFAnimation_Create()
 */
MF_API MFMatrix *MFAnimation_CalculateMatrices(MFAnimation *pAnimation, MFMatrix *pLocalToWorld);

/**
 * Get the animations frame range.
 * Gets the animations valid frame range.
 * @param pAnimation Animation instance.
 * @param pStartTime Pointer to a float that will receive the start time. Can be NULL.
 * @param pEndTime Pointer to a float that will receive the end time. Can be NULL.
 * @return None.
 * @see MFAnimation_SetFrame()
 */
MF_API void MFAnimation_GetFrameRange(const MFAnimation *pAnimation, float *pStartTime, float *pEndTime);

/**
 * Set the current frame.
 * Sets the current frame time.
 * @param pAnimation Animation instance.
 * @param frameTime Frame time to be set as the current frame.
 * @return None.
 * @see MFAnimation_GetFrameRange()
 */
MF_API void MFAnimation_SetFrame(MFAnimation *pAnimation, float frameTime);

#endif

/** @} */
