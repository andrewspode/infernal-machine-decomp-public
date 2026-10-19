#include "rdLight.h"

#include <rdroid/Math/rdMath.h>
#include <rdroid/Math/rdVector.h>

#include <std/General/stdMemory.h>

/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(rdLight_countfit_1, 15)
CF_PROTO(rdLight_countfit_2, 15)
CF_PROTO(rdLight_countfit_3, 15)
CF_PROTO(rdLight_countfit_4, 15)
CF_PROTO(rdLight_countfit_5, 15)
CF_PROTO(rdLight_countfit_6, 15)
CF_PROTO(rdLight_countfit_7, 15)
CF_PROTO(rdLight_countfit_8, 15)
CF_PROTO(rdLight_countfit_9, 15)
CF_PROTO(rdLight_countfit_10, 15)
CF_PROTO(rdLight_countfit_11, 6)
/* end count-fit */



void J3DAPI rdLight_FreeEntry(rdLight* pLight); // debug build: rdLight_Free calls it before its definition

/* Linker-discarded (no caller in either image): the Indy debug build's rdLight_New (0x00518890), Free (0x00518962) and the
   empty FreeEntry (0x005189B0), restored from its code; Jedi Knight's rdLight.c has the same four in this order. */
rdLight* J3DAPI rdLight_New(void)
{
    rdLight* pLight;

#line 65
    pLight = (rdLight*)STDMALLOC(sizeof(rdLight));
    if ( !pLight )
    {
#line 68
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Error allocating memory for light.\n", 0, 0, 0, 0);
        return NULL;
    }

    rdLight_NewEntry(pLight);
    return pLight;
}

int J3DAPI rdLight_NewEntry(rdLight* pLight)
{
    // type 2 - probably point light
    // Mots 3 - probably spot light
    pLight->type = 2;

    pLight->bEnabled = 1;

    pLight->direction.x = 0.0f;
    pLight->direction.y = 0.0f;
    pLight->direction.z = 0.0f;

    pLight->color.red   = 1.0f;
    pLight->color.green = 1.0f;
    pLight->color.blue  = 1.0f;
    pLight->color.alpha = 0.0f;

    pLight->minRadius = 0.0f;
    pLight->maxRadius = 0.0f;
    return 1;
}

void J3DAPI rdLight_Free(rdLight* pLight)
{
    if ( pLight )
    {
        rdLight_FreeEntry(pLight);
        stdMemory_Free(pLight);
    }
    else
    {
#line 136
        stdPrintf(rdroid_pHS->pErrorPrint, J3D_FILE, __LINE__, "Warning: Attempt to free NULL light ptr.\n", 0, 0, 0, 0);
    }
}

void J3DAPI rdLight_FreeEntry(rdLight* pLight)
{
}


void J3DAPI rdLight_CalcVertexIntensities(const rdLight** apLights, const rdVector3* aLightPos, size_t numLights, const rdVector3* aVertexNormal, const rdVector3* aVertices, const rdVector4* aVertexColors, rdVector4* aColors, size_t numVertices, float attenuation)
{
    int i;
    for ( i = numVertices - 1; i >= 0; --i )
    {
        int j;

        rdVector_Copy4(&aColors[i], &aVertexColors[i]);

        for ( j = numLights - 1; j >= 0; --j )
        {
            float dist;

            const rdLight* pLight = apLights[j];

            rdVector3 vecDir;
            rdVector_Sub3(&vecDir, &aLightPos[j], &aVertices[i]);
            dist = rdVector_Len3(&vecDir);
            if ( dist < pLight->minRadius )
            {
                float dot;

                rdVector_Normalize3Acc(&vecDir);
                dot = rdVector_Dot3(&aVertexNormal[i], &vecDir);
                if ( dot > 0.0f )
                {
                    if ( dist < pLight->maxRadius )
                    {
                        aColors[i].red   += pLight->color.red * dot;
                        aColors[i].green += pLight->color.green * dot;
                        aColors[i].blue  += pLight->color.blue * dot;
                    }
                    else
                    {
                        float drad;
                        float delta;
                        float atten;
                        drad  = pLight->minRadius - pLight->maxRadius;
                        delta = dist - pLight->maxRadius;
                        atten = delta / drad;

                        aColors[i].red   += (pLight->color.red - atten * pLight->color.red) * dot;
                        aColors[i].green += (pLight->color.green - atten * pLight->color.green) * dot;
                        aColors[i].blue  += (pLight->color.blue - atten * pLight->color.blue) * dot;
                    }
                }
            }
        }
    }
}

/* Linker-discarded (no caller in either image): debug 0x00518C37, between CalcVertexIntensities and CalcFaceIntensity,
   the distance-attenuated variant: every light inside minRadius adds its colour minus dist * attenuation per channel.
   Name from Jones3DRE. */
void J3DAPI rdLight_CalcDistVertexIntensities(const rdLight** apLights, const rdVector3* aLightPos, size_t numLights, const rdVector3* aVertexNormal, const rdVector3* aVertices, const rdVector4* aVertexColors, rdVector4* aColors, size_t numVertices, float attenuation)
{
    size_t i;
    size_t j;
    const rdLight* pLight;
    rdVector3 vecDir;
    float dist;
    float atten;
    float red;
    float green;
    float blue;

    for ( i = 0; i < numVertices; ++i )
    {
        rdVector_Copy4(&aColors[i], &aVertexColors[i]);

        for ( j = 0; j < numLights; ++j )
        {
            pLight = apLights[j];

            rdVector_Sub3(&vecDir, &aLightPos[j], &aVertices[i]);
            dist = rdVector_Len3(&vecDir);
            if ( dist < pLight->minRadius )
            {
                atten = dist * attenuation;
                red   = pLight->color.red - atten;
                green = pLight->color.green - atten;
                blue  = pLight->color.blue - atten;

                aColors[i].red   += red;
                aColors[i].green += red; // sic: the debug code adds the red value to all three channels (green/blue are computed, never read)
                aColors[i].blue  += red;
            }
        }
    }
}


void J3DAPI rdLight_CalcFaceIntensity(const rdLight** apLights, const rdVector3* apLightPos, size_t numLights, const rdFace* pFace, const rdVector3* pNormal, const rdVector3* apVertices, float attenuation, rdVector4* pOutColor)
{
    const rdLight* pLight;
    float dist;
    size_t i;
    rdVector3 dir;

    rdVector_Set4(pOutColor, 0.0f, 0.0f, 0.0f, 0.0f);

    for ( i = 0; i < numLights; ++i )
    {
        pLight = apLights[i];
        if ( pLight->bEnabled )
        {
            rdVector_Sub3(&dir, &apLightPos[i], &apVertices[*pFace->aVertices]);

            dist = rdMath_DistancePointToPlane(&apLightPos[i], pNormal, &apVertices[*pFace->aVertices]);
            if ( dist < pLight->minRadius )
            {
                float dot;

                rdVector_Normalize3Acc(&dir);
                dot = rdVector_Dot3(&dir, pNormal);
                if ( dot > 0.0f )
                {
                    float atten = dist * attenuation;
                    pOutColor->red   += (pLight->color.red - atten) * dot;
                    pOutColor->green += (pLight->color.green - atten) * dot;
                    pOutColor->blue  += (pLight->color.blue - atten) * dot;
                }
            }
        }
    }
}

/* Linker-discarded (no caller in either image): debug 0x00518F65, between CalcFaceIntensity and GetIntensity; the face
   variant of the distance-attenuated colour add, without the direction test. Name from Jones3DRE. */
void J3DAPI rdLight_CalcDistFaceIntensity(const rdLight** apLights, const rdVector3* apLightPos, size_t numLights, const rdFace* pFace, const rdVector3* apVertices, float attenuation, rdVector4* pOutColor)
{
    size_t i;
    const rdLight* pLight;
    float dist;
    float atten;

    rdVector_Set4(pOutColor, 0.0f, 0.0f, 0.0f, 0.0f);

    for ( i = 0; i < numLights; ++i )
    {
        pLight = apLights[i];
        if ( pLight->bEnabled )
        {
            dist = rdMath_DistancePointToPlane(&apLightPos[i], &pFace->normal, &apVertices[*pFace->aVertices]);
            if ( dist < pLight->minRadius )
            {
                atten = dist * attenuation;
                pOutColor->red   += pLight->color.red - atten;
                pOutColor->green += pLight->color.green - atten;
                pOutColor->blue  += pLight->color.blue - atten;
            }
        }
    }
}

float J3DAPI rdLight_GetIntensity(const rdVector4* pLight)
{
    float intensity;
    intensity = pLight->red + pLight->green + pLight->blue;
    intensity = intensity / 3.0f;
    return intensity;
}
