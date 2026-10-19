#ifndef RDROID_RDPARTICLE_H
#define RDROID_RDPARTICLE_H
#include <j3dcore/j3d.h>
#include <rdroid/types.h>
#include <std/types.h>

#define RDPARTICLE_MAXVERTS 256

J3D_EXTERN_C_START

void J3DAPI rdParticle_RegisterLoader(rdParticle* (J3DAPI* pFunc)(const char*));
rdParticle* J3DAPI rdParticle_New(size_t num, float size, rdMaterial* pMat, rdLightMode lightMode);
int J3DAPI rdParticle_NewEntry(rdParticle* pParticle, size_t num, float size, rdMaterial* pMaterial, rdLightMode lightMode);
rdParticle* J3DAPI rdParticle_Duplicate(const rdParticle* pOriginal);

void J3DAPI rdParticle_Free(rdParticle* pParticle);
void J3DAPI rdParticle_FreeEntry(rdParticle* pParticle);
rdParticle* J3DAPI rdParticle_Load(char* pFilename);

int J3DAPI rdParticle_LoadEntry(const char* pFilename, rdParticle* pParticle);

int J3DAPI rdParticle_Draw(const rdThing* pParticle, const rdMatrix34* pOrient);

J3D_EXTERN_C_END
#endif // RDROID_RDPARTICLE_H
