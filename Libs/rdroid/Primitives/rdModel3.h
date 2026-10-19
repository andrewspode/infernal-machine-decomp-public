#ifndef RDROID_RDMODEL3_H
#define RDROID_RDMODEL3_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>

J3D_EXTERN_C_START

#define RDMODEL3_MAX_VERTICES      1024
#define RDMODEL3_MAX_TEXVRTICES    768

#define RDMODEL3_MAX_FACES         1528
#define RDMODEL3_MAX_FACE_VERTICES RDCACHE_MAXFACEVERTICES 

extern int rdModel3_numDrawnFaces;

extern int rdModel3_numDrawnAlphaFaces;

rdModel3LoaderFunc J3DAPI rdModel3_RegisterLoader(rdModel3LoaderFunc pfFunc);

void J3DAPI rdModel3_NewEntry(rdModel3* pModel);
rdModel3* J3DAPI rdModel3_Load(const char* pName);
int J3DAPI rdModel3_LoadEntry(const char* pFilename, rdModel3* pModel3);
int J3DAPI rdModel3_Write(const char* pFilename, const rdModel3* pModel, const char* pCratedName);
void J3DAPI rdModel3_Free(rdModel3* pModel3);

void J3DAPI rdModel3_FreeEntry(rdModel3* pModel3);
void J3DAPI rdModel3_FreeEntryGeometryOnly(rdModel3* pModel);
int J3DAPI rdModel3_Validate(const rdModel3* pModel3);
void J3DAPI rdModel3_CalcRadii(rdModel3* pModel3);

void J3DAPI rdModel3_BuildExpandedRadius(const rdModel3* pModel, const rdModel3HNode* pNode, const rdMatrix34* mat);
void J3DAPI rdModel3_CalcFaceNormals(rdModel3* pModel3);
void J3DAPI rdModel3_CalcVertexNormals(rdModel3* pModel);
rdModel3HNode* J3DAPI rdModel3_FindNamedNode(const char* pName, rdModel3* pModel3);

int J3DAPI rdModel3_GetMeshMatrix(rdThing* pThing, const rdMatrix34* orient, unsigned int nodeNum, rdMatrix34* meshOrient);
int J3DAPI rdModel3_ReplaceMesh(rdModel3* pModel, unsigned int geosetNum, unsigned int meshNum, const rdModel3Mesh* pSrcMesh);
int J3DAPI rdModel3_SwapMesh(rdModel3* pModel1, unsigned int meshNum1, rdModel3* pModel2, int meshNum2);

void J3DAPI rdModel3_GetThingColor(const rdThing* pThing, rdVector4* pDestColor);
void J3DAPI rdModel3_SetModelColor(rdModel3* pModel, const rdVector4* pColor);
void J3DAPI rdModel3_SetThingColor(rdThing* prdThing, const rdVector4* pColor);
void J3DAPI rdModel3_SetNodeColor(const rdModel3* pModel, rdModel3GeoSet* pGeoSet, const rdModel3HNode* pNode, const rdVector4* pColor);
void J3DAPI rdModel3_SetNodeColorForNonAmputatedJoints(const rdThing* prdThing, rdModel3GeoSet* pGeoSet, const rdModel3HNode* pNode, const rdVector4* pColor);

int J3DAPI rdModel3_Draw(rdThing* pThing, const rdMatrix34* pPlacement);
void J3DAPI rdModel3_DrawHNode(const rdModel3GeoSet* prdGeo, const rdModel3HNode* pNode);
void J3DAPI rdModel3_DrawMesh(const rdModel3Mesh* pMesh, const rdMatrix34* pOrient);
void J3DAPI rdModel3_DrawFace(const rdFace* pFace, const rdVector3* aTransformedVertices, int bIsBackFace, const rdVector4* pMeshColor);
void J3DAPI rdModel3_EnableFogRendering(int bEnabled);

J3D_EXTERN_C_END
#endif // RDROID_RDMODEL3_H
