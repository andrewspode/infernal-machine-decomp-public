#ifndef RDROID_RDMATRIX_H
#define RDROID_RDMATRIX_H
#include <j3dcore/j3d.h>

#include <rdroid/types.h>

#include <std/types.h>

J3D_EXTERN_C_START

extern const rdMatrix34 rdroid_identMatrix34;
extern const rdMatrix44 rdroid_identMatrix44;

void J3DAPI rdMatrix_Copy34(rdMatrix34* dest, const rdMatrix34* src);

void J3DAPI rdMatrix_Build34(rdMatrix34* mat, const rdVector3* pyr, const rdVector3* pos);
void J3DAPI rdMatrix_BuildFromLook34(rdMatrix34* mat, const rdVector3* look);
void J3DAPI rdMatrix_BuildCamera34(rdMatrix34* mat, const rdVector3* pyr, const rdVector3* pos);
void J3DAPI rdMatrix_InvertOrtho34(rdMatrix34* dest, const rdMatrix34* src);
void J3DAPI rdMatrix_Build44(rdMatrix44* mat, const rdVector3* pyr, const rdVector3* pos);
void J3DAPI rdMatrix_BuildRotate34(rdMatrix34* mat, const rdVector3* pyr);
void J3DAPI rdMatrix_BuildInverseRotate34(rdMatrix34* mat, const rdVector3* pyr);
void J3DAPI rdMatrix_BuildRotate44(rdMatrix44* mat, const rdVector3* pyr);
void J3DAPI rdMatrix_BuildTranslate34(rdMatrix34* mat, const rdVector3* vec);
void J3DAPI rdMatrix_BuildTranslate44(rdMatrix44* mat, const rdVector3* vec);
void J3DAPI rdMatrix_BuildScale34(rdMatrix34* mat, const rdVector3* scale);
void J3DAPI rdMatrix_BuildScale44(rdMatrix44* mat, const rdVector3* scale);
void J3DAPI rdMatrix_BuildFromVectorAngle34(rdMatrix34* mat, const rdVector3* vec, float angle);

void J3DAPI rdMatrix_Normalize34(rdMatrix34* mat);

void J3DAPI rdMatrix_Identity34(rdMatrix34* mat);
void J3DAPI rdMatrix_Identity44(rdMatrix44* mat);
void J3DAPI rdMatrix_Copy44(rdMatrix44* dest, const rdMatrix44* src);
void J3DAPI rdMatrix_Copy34to44(rdMatrix44* dest, const rdMatrix34* src);
void J3DAPI rdMatrix_Copy44to34(rdMatrix34* dest, const rdMatrix44* src);
void J3DAPI rdMatrix_Transpose44(rdMatrix44* dest, const rdMatrix44* src);

void J3DAPI rdMatrix_LookAt(rdMatrix34* mat, const rdVector3* eyePos, const rdVector3* lookPos, float angle);
void J3DAPI rdMatrix_ExtractAngles34(const rdMatrix34* mat, rdVector3* pyr);

void J3DAPI rdMatrix_Multiply34(rdMatrix34* dest, const rdMatrix34* a, const rdMatrix34* b);
void J3DAPI rdMatrix_Multiply44(rdMatrix44* dest, const rdMatrix44* a, const rdMatrix44* b);
void J3DAPI rdMatrix_PreMultiply34(rdMatrix34* a, const rdMatrix34* b);
void J3DAPI rdMatrix_PreMultiply44(rdMatrix44* a, const rdMatrix44* b);
void J3DAPI rdMatrix_PostMultiply34(rdMatrix34* a, const rdMatrix34* b);
void J3DAPI rdMatrix_PostMultiply44(rdMatrix44* a, const rdMatrix44* b);

void J3DAPI rdMatrix_PreRotate34(rdMatrix34* mat, const rdVector3* pyr);
void J3DAPI rdMatrix_PreRotate44(rdMatrix44* mat, const rdVector3* pyr);
void J3DAPI rdMatrix_PostRotate34(rdMatrix34* mat, const rdVector3* vecPYR);
void J3DAPI rdMatrix_PostRotate44(rdMatrix44* mat, const rdVector3* pyr);

void J3DAPI rdMatrix_PreTranslate34(rdMatrix34* mat, const rdVector3* vec);
void J3DAPI rdMatrix_PreTranslate44(rdMatrix44* mat, const rdVector3* vec);
void J3DAPI rdMatrix_PostTranslate34(rdMatrix34* pMat, const rdVector3* pVec);
void J3DAPI rdMatrix_PostTranslate44(rdMatrix44* mat, const rdVector3* vec);

void J3DAPI rdMatrix_PreScale34(rdMatrix34* mat, const rdVector3* vec);
void J3DAPI rdMatrix_PreScale44(rdMatrix44* mat, const rdVector4* vec);
void J3DAPI rdMatrix_PostScale34(rdMatrix34* mat, const rdVector3* vec);
void J3DAPI rdMatrix_PostScale44(rdMatrix44* mat, const rdVector4* vec);

void J3DAPI rdMatrix_SetRowVector34(rdMatrix34* mat, int row, const rdVector3* vec);
void J3DAPI rdMatrix_SetRowVector44(rdMatrix44* mat, int row, const rdVector4* vec);
void J3DAPI rdMatrix_GetRowVector34(rdMatrix34* mat, int row, rdVector3* vec);
void J3DAPI rdMatrix_GetRowVector44(rdMatrix44* mat, int row, rdVector4* vec);

void J3DAPI rdMatrix_TransformVector34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformVectorOrtho34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformVector34Acc(rdVector3* dest, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformVector44(rdVector4* dest, const rdVector4* src, const rdMatrix44* mat);
void J3DAPI rdMatrix_TransformVector44Acc(rdVector4* dest, const rdMatrix44* mat);

void J3DAPI rdMatrix_TransformPoint34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformPoint34Acc(rdVector3* dest, const rdMatrix34* mat);
void J3DAPI rdMatrix_TransformPointList34(const rdMatrix34* mat, const rdVector3* aSrc, rdVector3* aDest, size_t size);



#ifndef RDMATRIX_NO_INLINE
/* A memcpy macro, not an __inline function: the debug build has no parameter home at its sites. */
#define rdMatrix_Copy34(dest, src) memcpy((dest), (src), sizeof(rdMatrix34))
#endif

J3D_EXTERN_C_END
#endif // RDROID_RDMATRIX_H
