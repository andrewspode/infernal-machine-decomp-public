#ifndef RDROID_RDPOLYLINE_H
#define RDROID_RDPOLYLINE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

J3D_EXTERN_C_START

rdPolyline* J3DAPI rdPolyline_New(const char* pName, const char* pMatFilename, const char* pMatFilename2, float length, float baseRadius, float tipRadius, rdGeometryMode geoMode, rdLightMode lightMode, const rdVector4* pColor);
int J3DAPI rdPolyline_NewEntry(rdPolyline* pLine, const char* pName, const char* pMatFilename, float length, float baseRadius, float tipRadius, rdGeometryMode geoMode, rdLightMode lightMode, const rdVector4* pColor);
void J3DAPI rdPolyline_Free(rdPolyline* pPolyline);
void J3DAPI rdPolyline_FreeEntry(rdPolyline* pPolyline);
int J3DAPI rdPolyline_Draw(const rdThing* pLine, const rdMatrix34* pOrient);
void J3DAPI rdPolyline_DrawFace(const rdThing* pLine, const rdFace* pFace, const rdVector3* aVertices, const rdVector2* aUVs);

J3D_EXTERN_C_END
#endif // RDROID_RDPOLYLINE_H
