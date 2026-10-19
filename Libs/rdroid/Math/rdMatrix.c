#define RDMATRIX_NO_INLINE

#include "rdMatrix.h"
/* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(rdMatrix_countfit_40, 2) /* end count-fit */
#include "rdVector.h"
#include <rdroid/Main/rdroid.h>

#include <std/General/stdMath.h>

// File-local absolute value (the debug build expands it inline in LookAt; name inferred).
#define RDMATRIX_ABS(x) ((x) < 0.0f ? -(x) : (x))

/*
 * Every product in this file is RD_MULTIPLY(a, b) (rdroid.h), the product the
 * rdVector.h macros are written with, expanding to `(((a) * (b)))` (compare
 * rdVector_Dot3 and rdVector_Scale3): the out-of-line matrix routines are
 * written in the header's style, like rdVector.c's own Dot3 (a paren
 * convention). VC5 keeps each parenthesised leaf and each doubly
 * wrapped product as its own tree node, which is what gives retail's x87
 * preload/fold map in every row (Multiply34, Build34 and BuildRotate34
 * byte-exact with it). BuildFromVectorAngle34's two
 * three-factor rows multiply the macro's product by a bare oneMinusCos.
 */

void J3DAPI rdMatrix_Build34(rdMatrix34* res, const rdVector3* pyr, const rdVector3* pos)
{
    float xSin;
    float xCos;
    float ySin;
    float yCos;
    float zSin;
    float zCos;
    float yzSin;
    float zSinYCos;
    float zCosYSin;
    float yzCos;

    stdMath_SinCos(pyr->pitch, &xSin, &xCos);
    stdMath_SinCos(pyr->yaw, &ySin, &yCos);
    stdMath_SinCos(pyr->roll, &zSin, &zCos);

    yzCos = RD_MULTIPLY(yCos, zCos);
    yzSin = RD_MULTIPLY(ySin, zSin);
    zSinYCos = RD_MULTIPLY(yCos, zSin);
    zCosYSin = RD_MULTIPLY(ySin, zCos);

    res->rvec.x = RD_MULTIPLY(-yzSin, xSin) + yzCos;
    res->rvec.y = RD_MULTIPLY(zSinYCos, xSin) + zCosYSin;
    res->rvec.z = RD_MULTIPLY(-zSin, xCos);
    res->lvec.x = RD_MULTIPLY(-ySin, xCos);
    res->lvec.y = RD_MULTIPLY(yCos, xCos);
    res->lvec.z = xSin;
    res->uvec.x = RD_MULTIPLY(zCosYSin, xSin) + zSinYCos;
    res->uvec.y = RD_MULTIPLY(-xSin, yzCos) + yzSin;
    res->uvec.z = RD_MULTIPLY(zCos, xCos);

    rdVector_Copy3(&res->dvec, pos);
}

void J3DAPI rdMatrix_BuildFromLook34(rdMatrix34* mat, const rdVector3* look)
{
#line 92
    RD_ASSERTREL(mat && look);
    rdVector_Copy3(&mat->lvec, look);
    rdVector_Cross3(&mat->rvec, &mat->lvec, &rdroid_zVector3);
    if ( rdVector_Normalize3Acc(&mat->rvec) == 0.0f )
    {
        rdVector_Cross3(&mat->uvec, &rdroid_xVector3, &mat->lvec);
        rdVector_Normalize3Acc(&mat->uvec);
        rdVector_Cross3(&mat->rvec, &mat->lvec, &mat->uvec);
    }
    else
    {
        rdVector_Cross3(&mat->uvec, &mat->rvec, &mat->lvec);
    }
}

// Linker-discarded; body from the debug build (0x508b7c, `__LINE__` 112, assert "mat && up"). The name is inferred.
void J3DAPI rdMatrix_BuildFromUp34(rdMatrix34* mat, const rdVector3* up)
{
#line 112
    RD_ASSERTREL(mat && up);
    rdVector_Copy3(&mat->uvec, up);
    mat->rvec.x = RD_MULTIPLY(mat->uvec.z, rdroid_zVector3.y) - RD_MULTIPLY(mat->uvec.y, rdroid_zVector3.z);
    mat->rvec.y = RD_MULTIPLY(mat->uvec.x, rdroid_zVector3.z) - RD_MULTIPLY(mat->uvec.z, rdroid_zVector3.x);
    mat->rvec.z = RD_MULTIPLY(mat->uvec.y, rdroid_zVector3.x) - RD_MULTIPLY(mat->uvec.x, rdroid_zVector3.y);
    if ( rdVector_Normalize3Acc(&mat->rvec) == 0.0f )
    {
        rdVector_Cross3(&mat->lvec, &mat->uvec, &rdroid_xVector3);
        rdVector_Normalize3Acc(&mat->lvec);
        rdVector_Cross3(&mat->rvec, &mat->lvec, &mat->uvec);
    }
    else
    {
        rdVector_Cross3(&mat->lvec, &mat->uvec, &mat->rvec);
    }
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_BuildCamera34(rdMatrix34* mat, const rdVector3* pyr, const rdVector3* pos)
{
    float xSin;
    float xCos;
    float ySin;
    float yCos;
    float zSin;
    float zCos;
    float yzSin;
    float zSinYCos;
    float zCosYSin;
    float yzCos;

    stdMath_SinCos(-pyr->pitch, &xSin, &xCos);
    stdMath_SinCos(-pyr->yaw, &ySin, &yCos);
    stdMath_SinCos(-pyr->roll, &zSin, &zCos);

    yzCos = RD_MULTIPLY(yCos, zCos);
    yzSin = RD_MULTIPLY(ySin, zSin);
    zSinYCos = RD_MULTIPLY(yCos, zSin);
    zCosYSin = RD_MULTIPLY(ySin, zCos);

    mat->rvec.x = RD_MULTIPLY(yzSin, xSin) + yzCos;
    mat->rvec.y = RD_MULTIPLY(ySin, xCos);
    mat->rvec.z = -zSinYCos + RD_MULTIPLY(zCosYSin, xSin);
    mat->lvec.x = -zCosYSin + RD_MULTIPLY(zSinYCos, xSin);
    mat->lvec.y = RD_MULTIPLY(xCos, yCos);
    mat->lvec.z = RD_MULTIPLY(yzCos, xSin) + yzSin;
    mat->uvec.x = RD_MULTIPLY(zSin, xCos);
    mat->uvec.y = -xSin;
    mat->uvec.z = RD_MULTIPLY(zCos, xCos);

    rdVector_Neg3(&mat->dvec, pos);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(rdMatrix_countfit_1, 30, 8) CF_PROTO(rdMatrix_countfit_9, 28) /* end count-fit */
void J3DAPI rdMatrix_InvertOrtho34(rdMatrix34* dest, const rdMatrix34* src)
{
    float x;
    float y;
    float z;

    dest->rvec.y = src->lvec.x;
    dest->rvec.z = src->uvec.x;
    dest->lvec.z = src->uvec.y;
    dest->lvec.x = src->rvec.y;
    dest->uvec.x = src->rvec.z;
    dest->uvec.y = src->lvec.z;
    dest->rvec.x = src->rvec.x;
    dest->lvec.y = src->lvec.y;
    dest->uvec.z = src->uvec.z;

    x = src->dvec.x;
    y = src->dvec.y;
    z = src->dvec.z;
    dest->dvec.x = -(RD_MULTIPLY(x, src->rvec.x) + RD_MULTIPLY(src->rvec.y, y) + RD_MULTIPLY(src->rvec.z, z));
    dest->dvec.y = -(RD_MULTIPLY(src->lvec.x, x) + RD_MULTIPLY(src->lvec.y, y) + RD_MULTIPLY(src->lvec.z, z));
    dest->dvec.z = -(RD_MULTIPLY(src->uvec.x, x) + RD_MULTIPLY(src->uvec.y, y) + RD_MULTIPLY(src->uvec.z, z));
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_Build44(rdMatrix44* mat, const rdVector3* pyr, const rdVector3* pos)
{
    float xSin;
    float xCos;
    float ySin;
    float yCos;
    float zSin;
    float zCos;
    float yzSin;
    float zSinYCos;
    float zCosYSin;
    float yzCos;

    stdMath_SinCos(pyr->pitch, &xSin, &xCos);
    stdMath_SinCos(pyr->yaw, &ySin, &yCos);
    stdMath_SinCos(pyr->roll, &zSin, &zCos);

    yzCos = RD_MULTIPLY(yCos, zCos);
    yzSin = RD_MULTIPLY(ySin, zSin);
    zSinYCos = RD_MULTIPLY(yCos, zSin);
    zCosYSin = RD_MULTIPLY(ySin, zCos);

    mat->vA.x = RD_MULTIPLY(-yzSin, xSin) + yzCos;
    mat->vA.y = RD_MULTIPLY(zSinYCos, xSin) + zCosYSin;
    mat->vA.z = RD_MULTIPLY(-zSin, xCos);
    mat->vA.w = 0.0f;
    mat->vB.x = RD_MULTIPLY(-ySin, xCos);
    mat->vB.y = RD_MULTIPLY(yCos, xCos);
    mat->vB.z = xSin;
    mat->vB.w = 0.0f;
    mat->vC.x = RD_MULTIPLY(zCosYSin, xSin) + zSinYCos;
    mat->vC.y = RD_MULTIPLY(-xSin, yzCos) + yzSin;
    mat->vC.z = RD_MULTIPLY(zCos, xCos);
    mat->vC.w = 0.0f;

    rdVector_Copy3((rdVector3*)&mat->vD, pos);
    mat->vD.w = 1.0f;
}

void J3DAPI rdMatrix_BuildRotate34(rdMatrix34* mat, const rdVector3* pyr)
{
    float xSin;
    float xCos;
    float ySin;
    float yCos;
    float zSin;
    float zCos;
    float yzSin;
    float zSinYCos;
    float zCosYSin;
    float yzCos;

    stdMath_SinCos(pyr->x, &xSin, &xCos);
    stdMath_SinCos(pyr->y, &ySin, &yCos);
    stdMath_SinCos(pyr->z, &zSin, &zCos);

    yzCos = RD_MULTIPLY(yCos, zCos);
    yzSin = RD_MULTIPLY(ySin, zSin);
    zSinYCos = RD_MULTIPLY(yCos, zSin);
    zCosYSin = RD_MULTIPLY(ySin, zCos);

    mat->rvec.x = RD_MULTIPLY(-yzSin, xSin) + yzCos;
    mat->rvec.y = RD_MULTIPLY(zSinYCos, xSin) + zCosYSin;
    mat->rvec.z = RD_MULTIPLY(-zSin, xCos);
    mat->lvec.x = RD_MULTIPLY(-ySin, xCos);
    mat->lvec.y = RD_MULTIPLY(yCos, xCos);
    mat->lvec.z = xSin;
    mat->uvec.x = RD_MULTIPLY(zCosYSin, xSin) + zSinYCos;
    mat->uvec.y = RD_MULTIPLY(-xSin, yzCos) + yzSin;
    mat->uvec.z = RD_MULTIPLY(zCos, xCos);

    memset(&mat->dvec, 0, sizeof(mat->dvec));
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_BuildInverseRotate34(rdMatrix34* mat, const rdVector3* pyr)
{
    float xSin;
    float xCos;
    float ySin;
    float yCos;
    float zSin;
    float zCos;
    float yzSin;
    float zSinYCos;
    float zCosYSin;
    float yzCos;

    stdMath_SinCos(pyr->x, &xSin, &xCos);
    stdMath_SinCos(pyr->y, &ySin, &yCos);
    stdMath_SinCos(pyr->z, &zSin, &zCos);

    yzCos = RD_MULTIPLY(yCos, zCos);
    yzSin = RD_MULTIPLY(ySin, zSin);
    zSinYCos = RD_MULTIPLY(yCos, zSin);
    zCosYSin = RD_MULTIPLY(ySin, zCos);

    mat->rvec.x = RD_MULTIPLY(yzSin, xSin) + yzCos;
    mat->rvec.y = RD_MULTIPLY(ySin, xCos);
    mat->rvec.z = -zSinYCos + RD_MULTIPLY(xSin, zCosYSin);
    mat->lvec.x = -zCosYSin + RD_MULTIPLY(zSinYCos, xSin);
    mat->lvec.y = RD_MULTIPLY(xCos, yCos);
    mat->lvec.z = RD_MULTIPLY(yzCos, xSin) + yzSin;
    mat->uvec.x = RD_MULTIPLY(zSin, xCos);
    mat->uvec.y = -xSin;
    mat->uvec.z = RD_MULTIPLY(zCos, xCos);

    memset(&mat->dvec, 0, sizeof(mat->dvec));
}

void J3DAPI rdMatrix_BuildRotate44(rdMatrix44* mat, const rdVector3* pyr)
{
    float xSin;
    float xCos;
    float ySin;
    float yCos;
    float zSin;
    float zCos;
    float yzSin;
    float zSinYCos;
    float zCosYSin;
    float yzCos;

    stdMath_SinCos(pyr->x, &xSin, &xCos);
    stdMath_SinCos(pyr->y, &ySin, &yCos);
    stdMath_SinCos(pyr->z, &zSin, &zCos);

    yzCos = RD_MULTIPLY(yCos, zCos);
    yzSin = RD_MULTIPLY(ySin, zSin);
    zSinYCos = RD_MULTIPLY(yCos, zSin);
    zCosYSin = RD_MULTIPLY(ySin, zCos);

    mat->vA.x = RD_MULTIPLY(-yzSin, xSin) + yzCos;
    mat->vA.y = RD_MULTIPLY(zSinYCos, xSin) + zCosYSin;
    mat->vA.z = RD_MULTIPLY(-zSin, xCos);
    mat->vA.w = 0.0f;
    mat->vB.x = RD_MULTIPLY(-ySin, xCos);
    mat->vB.y = RD_MULTIPLY(yCos, xCos);
    mat->vB.z = xSin;
    mat->vB.w = 0.0f;
    mat->vC.x = RD_MULTIPLY(zCosYSin, xSin) + zSinYCos;
    mat->vC.y = RD_MULTIPLY(-xSin, yzCos) + yzSin;
    mat->vC.z = RD_MULTIPLY(zCos, xCos);
    mat->vC.w = 0.0f;

    memset(&mat->vD, 0, sizeof(rdVector3));
    mat->vD.w = 1.0f;
}

void J3DAPI rdMatrix_BuildTranslate34(rdMatrix34* mat, const rdVector3* vec)
{
    memcpy(mat, &rdroid_identMatrix34, sizeof(rdMatrix34));
    rdVector_Copy3(&mat->dvec, vec);
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_BuildTranslate44(rdMatrix44* mat, const rdVector3* vec)
{
    memcpy(mat, &rdroid_identMatrix44, sizeof(rdMatrix44));
    rdVector_Copy3((rdVector3*)&mat->vD, vec);
    mat->vD.w = 1.0f;
}

void J3DAPI rdMatrix_BuildScale34(rdMatrix34* mat, const rdVector3* scale)
{
    mat->rvec.x = scale->x;
    mat->rvec.y = 0.0f;
    mat->rvec.z = 0.0f;

    mat->lvec.x = 0.0f;
    mat->lvec.y = scale->y;
    mat->lvec.z = 0.0f;

    mat->uvec.x = 0.0f;
    mat->uvec.y = 0.0f;
    mat->uvec.z = scale->z;

    memset(&mat->dvec, 0, sizeof(mat->dvec));
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_BuildScale44(rdMatrix44* mat, const rdVector3* scale)
{
    mat->vA.x = scale->x;
    mat->vA.y = 0.0f;
    mat->vA.z = 0.0f;
    mat->vA.w = 0.0f;
    mat->vB.x = 0.0f;
    mat->vB.y = scale->y;
    mat->vB.z = 0.0f;
    mat->vA.w = 0.0f;
    mat->vC.x = 0.0f;
    mat->vC.y = 0.0f;
    mat->vC.z = scale->z;
    mat->vA.w = 1.0f;
    memset(&mat->vD, 0, sizeof(rdVector3));
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(rdMatrix_countfit_50, 30, 6) CF_PROTO(rdMatrix_countfit_56, 1) /* end count-fit */
void J3DAPI rdMatrix_BuildFromVectorAngle34(rdMatrix34* mat, const rdVector3* vec, float angle)
{
    float xx;
    float yy;
    float zz;
    float sinAngle;
    float cosAngle;
    float cyy;
    float cxx;
    float invZZ;
    float oneMinusCos;
    float xy;
    float yz;
    float xz;
    float xs;
    float ys;
    float zs;
    float omcxz;
    float omcyz;

    stdMath_SinCos(angle, &sinAngle, &cosAngle);

    if ( vec->z >= 1.0f )
    {
        mat->rvec.x = cosAngle;
        mat->lvec.y = cosAngle;
        mat->rvec.y = sinAngle;
        mat->lvec.x = -sinAngle;
        mat->rvec.z = 0.0f;
        mat->lvec.z = 0.0f;
        rdVector_Set3(&mat->uvec, 0.0f, 0.0f, 1.0f);
    }
    else if ( vec->z <= -1.0f )
    {
        mat->rvec.x = cosAngle;
        mat->lvec.y = cosAngle;
        mat->rvec.y = -sinAngle;
        mat->lvec.x = sinAngle;
        mat->rvec.z = 0.0f;
        mat->lvec.z = 0.0f;
        rdVector_Set3(&mat->uvec, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        oneMinusCos = 1.0f - cosAngle;
        xy = RD_MULTIPLY(vec->y, vec->x);
        yz = RD_MULTIPLY(vec->z, vec->y);
        xz = RD_MULTIPLY(vec->z, vec->x);
        xs = RD_MULTIPLY(sinAngle, vec->x);
        ys = RD_MULTIPLY(vec->y, sinAngle);
        zs = RD_MULTIPLY(vec->z, sinAngle);
        xx = vec->x * vec->x;
        yy = RD_MULTIPLY(vec->y, vec->y);
        zz = 1.0f - xx - yy;
        invZZ = 1.0f - zz;
        cxx = RD_MULTIPLY(cosAngle, xx);
        cyy = RD_MULTIPLY(cosAngle, yy);
        omcxz = RD_MULTIPLY(oneMinusCos, xz);
        omcyz = RD_MULTIPLY(oneMinusCos, yz);

        mat->rvec.x = D3DVAL((RD_MULTIPLY(cxx, zz) + cyy) / invZZ) + xx;
        mat->lvec.y = D3DVAL((RD_MULTIPLY(cyy, zz) + cxx) / invZZ) + yy;
        mat->uvec.z = zz + cxx + cyy;

        mat->rvec.y = D3DVAL(RD_MULTIPLY(oneMinusCos, xy) + zs);
        mat->lvec.x = D3DVAL(RD_MULTIPLY(oneMinusCos, xy) - zs);
        mat->rvec.z = omcxz - ys;
        mat->lvec.z = omcyz + xs;
        mat->uvec.x = omcxz + ys;
        mat->uvec.y = omcyz - xs;
    }

    rdVector_Set3(&mat->dvec, 0.0f, 0.0f, 0.0f);
}

void J3DAPI rdMatrix_LookAt(rdMatrix34* mat, const rdVector3* eyePos, const rdVector3* lookPos, float angle)
{
    rdMatrix34 curtmat;
    rdVector3 curdir;
    rdVector3 fwdVector;

    rdVector_Sub3(&mat->lvec, lookPos, eyePos);
    rdVector_Normalize3Acc(&mat->lvec);
    rdMatrix_BuildFromVectorAngle34(&curtmat, &mat->lvec, angle);

    if ( RDMATRIX_ABS(rdVector_Dot3(&mat->lvec, &rdroid_zVector3)) > 0.999 )
    {
        if ( mat->lvec.z > 0.0f )
        {
            rdVector_Neg3(&fwdVector, &rdroid_yVector3);
            rdMatrix_TransformVector34(&curdir, &fwdVector, &curtmat);
        }
        else
        {
            rdMatrix_TransformVector34(&curdir, &rdroid_yVector3, &curtmat);
        }
    }
    else
    {
        rdMatrix_TransformVector34(&curdir, &rdroid_zVector3, &curtmat);
    }

    rdVector_Cross3(&mat->rvec, &mat->lvec, &curdir);
    rdVector_Normalize3Acc(&mat->rvec);

    rdVector_Cross3(&mat->uvec, &mat->rvec, &mat->lvec);
    rdVector_Normalize3Acc(&mat->uvec);

    rdVector_Copy3(&mat->dvec, eyePos);
}

void J3DAPI rdMatrix_ExtractAngles34(const rdMatrix34* mat, rdVector3* pyr)
{
    rdVector3 vecFwd;
    rdVector3 vecRight;
    rdVector3 vecUp;
    rdVector3 sideXY;
    rdVector3 fwdXY;
    float fwdLenXY;
    float sideLenXY;
    float angle;
    float cosAngle;

    vecFwd.x = mat->lvec.x;
    vecFwd.y = mat->lvec.y;
    vecFwd.z = mat->lvec.z;

    vecUp.x = mat->uvec.x;
    vecUp.y = mat->uvec.y;
    vecUp.z = mat->uvec.z;

    rdVector_Neg3(&vecRight, &mat->rvec);

    fwdXY.x = vecFwd.x;
    fwdXY.y = vecFwd.y;
    fwdXY.z = 0.0f;
    fwdLenXY = sqrtf(RD_MULTIPLY(fwdXY.x, fwdXY.x) + RD_MULTIPLY(fwdXY.y, fwdXY.y) + RD_MULTIPLY(fwdXY.z, fwdXY.z));

    if ( fwdLenXY < 0.001f )
    {
        angle = 90.0f - stdMath_ArcSin3(-vecRight.x);
        if ( (vecRight.y > 0.0f && vecFwd.z > 0.0f) || (vecRight.y < 0.0f && vecFwd.z < 0.0f) )
        {
            angle = -angle;
        }

        pyr->roll = angle;
        pyr->yaw  = 0.0f;
    }
    else
    {
        angle = 90.0f - stdMath_ArcSin3(fwdXY.y / fwdLenXY);
        if ( vecFwd.x > 0.0f )
        {
            angle = -angle;
        }

        pyr->yaw = angle;
    }

    if ( fwdLenXY < 0.001f )
    {
        pyr->pitch = 90.0f;
    }
    else
    {
        cosAngle = rdVector_Dot3(&fwdXY, &vecFwd) / fwdLenXY;
        if ( cosAngle >= 1.0f )
        {
            pyr->pitch = 0.0f;
        }
        else
        {
            angle = 90.0f - stdMath_ArcSin3(cosAngle);
            pyr->pitch = angle;
        }
    }

    if ( vecFwd.z < 0.0f )
    {
        pyr->pitch = -pyr->pitch;
    }

    sideXY.x = -fwdXY.y;
    sideXY.y = fwdXY.x;
    sideXY.z = 0.0f;
    sideLenXY = sqrtf(RD_MULTIPLY(sideXY.x, sideXY.x) + RD_MULTIPLY(sideXY.y, sideXY.y) + RD_MULTIPLY(sideXY.z, sideXY.z));

    if ( fwdLenXY >= 0.001f )
    {
        cosAngle = rdVector_Dot3(&sideXY, &vecRight) / sideLenXY;
        if ( cosAngle >= 1.0f )
        {
            pyr->roll = 0.0f;
        }
        else if ( cosAngle <= -1.0f )
        {
            pyr->roll = 180.0f;
        }
        else
        {
            angle = 90.0f - stdMath_ArcSin3(cosAngle);
            pyr->roll = angle;
        }

        if ( vecRight.z < 0.0f )
        {
            pyr->roll = -pyr->roll;
        }
    }
}

void J3DAPI rdMatrix_Normalize34(rdMatrix34* mat)
{
    rdVector_Cross3(&mat->uvec, &mat->rvec, &mat->lvec);

    rdVector_Normalize3Acc(&mat->lvec);
    rdVector_Normalize3Acc(&mat->uvec);

    rdVector_Cross3(&mat->rvec, &mat->lvec, &mat->uvec);
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_Identity34(rdMatrix34* mat)
{
    memcpy(mat, &rdroid_identMatrix34, sizeof(rdMatrix34));
}

void J3DAPI rdMatrix_Identity44(rdMatrix44* mat)
{
    memcpy(mat, &rdroid_identMatrix44, sizeof(rdMatrix44));
}

void J3DAPI rdMatrix_Copy34(rdMatrix34* dest, const rdMatrix34* src)
{
    memcpy(dest, src, sizeof(rdMatrix34));
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_Copy44(rdMatrix44* dest, const rdMatrix44* src)
{
    memcpy(dest, src, sizeof(rdMatrix44));
}

void J3DAPI rdMatrix_Copy34to44(rdMatrix44* dest, const rdMatrix34* src)
{
    dest->vA.x = src->rvec.x;
    dest->vA.y = src->rvec.y;
    dest->vA.z = src->rvec.z;
    dest->vA.w = 0.0f;
    dest->vB.x = src->lvec.x;
    dest->vB.y = src->lvec.y;
    dest->vB.z = src->lvec.z;
    dest->vB.w = 0.0f;
    dest->vC.x = src->uvec.x;
    dest->vC.y = src->uvec.y;
    dest->vC.z = src->uvec.z;
    dest->vC.w = 0.0f;
    dest->vD.x = src->dvec.x;
    dest->vD.y = src->dvec.y;
    dest->vD.z = src->dvec.z;
    dest->vD.w = 1.0f;
}

void J3DAPI rdMatrix_Copy44to34(rdMatrix34* dest, const rdMatrix44* src)
{
    dest->rvec.x = src->vA.x;
    dest->rvec.y = src->vA.y;
    dest->rvec.z = src->vA.z;
    dest->lvec.x = src->vB.x;
    dest->lvec.y = src->vB.y;
    dest->lvec.z = src->vB.z;
    dest->uvec.x = src->vC.x;
    dest->uvec.y = src->vC.y;
    dest->uvec.z = src->vC.z;
    dest->dvec.x = src->vD.x;
    dest->dvec.y = src->vD.y;
    dest->dvec.z = src->vD.z;
}

void J3DAPI rdMatrix_Transpose44(rdMatrix44* dest, const rdMatrix44* src)
{
    rdMatrix44 tmp;

    tmp.vA.x = src->vA.x;
    tmp.vA.y = src->vB.x;
    tmp.vA.z = src->vC.x;
    tmp.vA.w = src->vD.x;
    tmp.vB.x = src->vA.y;
    tmp.vB.y = src->vB.y;
    tmp.vB.z = src->vC.y;
    tmp.vB.w = src->vD.y;
    tmp.vC.x = src->vA.z;
    tmp.vC.y = src->vB.z;
    tmp.vC.z = src->vC.z;
    tmp.vC.w = src->vD.z;
    tmp.vD.x = src->vA.w;
    tmp.vD.y = src->vB.w;
    tmp.vD.z = src->vC.w;
    tmp.vD.w = src->vD.w;
    memcpy(dest, &tmp, sizeof(rdMatrix44));
}

void J3DAPI rdMatrix_Multiply34(rdMatrix34* dest, const rdMatrix34* a, const rdMatrix34* b)
{
#line 1024
    RD_ASSERTREL(((dest!=a)&&(dest!=b)));

    dest->rvec.x = RD_MULTIPLY(a->rvec.x, b->rvec.x) + RD_MULTIPLY(a->lvec.x, b->rvec.y) + RD_MULTIPLY(a->uvec.x, b->rvec.z);
    dest->rvec.y = RD_MULTIPLY(a->rvec.y, b->rvec.x) + RD_MULTIPLY(a->lvec.y, b->rvec.y) + RD_MULTIPLY(a->uvec.y, b->rvec.z);
    dest->rvec.z = RD_MULTIPLY(a->rvec.z, b->rvec.x) + RD_MULTIPLY(a->lvec.z, b->rvec.y) + RD_MULTIPLY(a->uvec.z, b->rvec.z);

    dest->lvec.x = RD_MULTIPLY(a->rvec.x, b->lvec.x) + RD_MULTIPLY(a->lvec.x, b->lvec.y) + RD_MULTIPLY(a->uvec.x, b->lvec.z);
    dest->lvec.y = RD_MULTIPLY(a->rvec.y, b->lvec.x) + RD_MULTIPLY(a->lvec.y, b->lvec.y) + RD_MULTIPLY(a->uvec.y, b->lvec.z);
    dest->lvec.z = RD_MULTIPLY(a->rvec.z, b->lvec.x) + RD_MULTIPLY(a->lvec.z, b->lvec.y) + RD_MULTIPLY(a->uvec.z, b->lvec.z);

    dest->uvec.x = RD_MULTIPLY(a->rvec.x, b->uvec.x) + RD_MULTIPLY(a->lvec.x, b->uvec.y) + RD_MULTIPLY(a->uvec.x, b->uvec.z);
    dest->uvec.y = RD_MULTIPLY(a->rvec.y, b->uvec.x) + RD_MULTIPLY(a->lvec.y, b->uvec.y) + RD_MULTIPLY(a->uvec.y, b->uvec.z);
    dest->uvec.z = RD_MULTIPLY(a->rvec.z, b->uvec.x) + RD_MULTIPLY(a->lvec.z, b->uvec.y) + RD_MULTIPLY(a->uvec.z, b->uvec.z);

    dest->dvec.x = RD_MULTIPLY(a->rvec.x, b->dvec.x) + RD_MULTIPLY(a->lvec.x, b->dvec.y) + RD_MULTIPLY(a->uvec.x, b->dvec.z) + a->dvec.x;
    dest->dvec.y = RD_MULTIPLY(a->rvec.y, b->dvec.x) + RD_MULTIPLY(a->lvec.y, b->dvec.y) + RD_MULTIPLY(a->uvec.y, b->dvec.z) + a->dvec.y;
    dest->dvec.z = RD_MULTIPLY(a->rvec.z, b->dvec.x) + RD_MULTIPLY(a->lvec.z, b->dvec.y) + RD_MULTIPLY(a->uvec.z, b->dvec.z) + a->dvec.z;
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_Multiply44(rdMatrix44* dest, const rdMatrix44* a, const rdMatrix44* b)
{
#line 1060
    RD_ASSERTREL(((dest!=a)&&(dest!=b)));

    dest->vA.x = RD_MULTIPLY(a->vA.x, b->vA.x) + RD_MULTIPLY(a->vB.x, b->vA.y) + RD_MULTIPLY(a->vC.x, b->vA.z) + RD_MULTIPLY(a->vD.x, b->vA.w);
    dest->vA.y = RD_MULTIPLY(a->vA.y, b->vA.x) + RD_MULTIPLY(a->vB.y, b->vA.y) + RD_MULTIPLY(a->vC.y, b->vA.z) + RD_MULTIPLY(a->vD.y, b->vA.w);
    dest->vA.z = RD_MULTIPLY(a->vA.z, b->vA.x) + RD_MULTIPLY(a->vB.z, b->vA.y) + RD_MULTIPLY(a->vC.z, b->vA.z) + RD_MULTIPLY(a->vD.z, b->vA.w);
    dest->vA.w = RD_MULTIPLY(a->vA.w, b->vA.x) + RD_MULTIPLY(a->vB.w, b->vA.y) + RD_MULTIPLY(a->vC.w, b->vA.z) + RD_MULTIPLY(a->vD.w, b->vA.w);

    dest->vB.x = RD_MULTIPLY(a->vA.x, b->vB.x) + RD_MULTIPLY(a->vB.x, b->vB.y) + RD_MULTIPLY(a->vC.x, b->vB.z) + RD_MULTIPLY(a->vD.x, b->vB.w);
    dest->vB.y = RD_MULTIPLY(a->vA.y, b->vB.x) + RD_MULTIPLY(a->vB.y, b->vB.y) + RD_MULTIPLY(a->vC.y, b->vB.z) + RD_MULTIPLY(a->vD.y, b->vB.w);
    dest->vB.z = RD_MULTIPLY(a->vA.z, b->vB.x) + RD_MULTIPLY(a->vB.z, b->vB.y) + RD_MULTIPLY(a->vC.z, b->vB.z) + RD_MULTIPLY(a->vD.z, b->vB.w);
    dest->vB.w = RD_MULTIPLY(a->vA.w, b->vB.x) + RD_MULTIPLY(a->vB.w, b->vB.y) + RD_MULTIPLY(a->vC.w, b->vB.z) + RD_MULTIPLY(a->vD.w, b->vB.w);

    dest->vC.x = RD_MULTIPLY(a->vA.x, b->vC.x) + RD_MULTIPLY(a->vB.x, b->vC.y) + RD_MULTIPLY(a->vC.x, b->vC.z) + RD_MULTIPLY(a->vD.x, b->vC.w);
    dest->vC.y = RD_MULTIPLY(a->vA.y, b->vC.x) + RD_MULTIPLY(a->vB.y, b->vC.y) + RD_MULTIPLY(a->vC.y, b->vC.z) + RD_MULTIPLY(a->vD.y, b->vC.w);
    dest->vC.z = RD_MULTIPLY(a->vA.z, b->vC.x) + RD_MULTIPLY(a->vB.z, b->vC.y) + RD_MULTIPLY(a->vC.z, b->vC.z) + RD_MULTIPLY(a->vD.z, b->vC.w);
    dest->vC.w = RD_MULTIPLY(a->vA.w, b->vC.x) + RD_MULTIPLY(a->vB.w, b->vC.y) + RD_MULTIPLY(a->vC.w, b->vC.z) + RD_MULTIPLY(a->vD.w, b->vC.w);

    dest->vD.x = RD_MULTIPLY(a->vA.x, b->vD.x) + RD_MULTIPLY(a->vB.x, b->vD.y) + RD_MULTIPLY(a->vC.x, b->vD.z) + RD_MULTIPLY(a->vD.x, b->vD.w);
    dest->vD.y = RD_MULTIPLY(a->vA.y, b->vD.x) + RD_MULTIPLY(a->vB.y, b->vD.y) + RD_MULTIPLY(a->vC.y, b->vD.z) + RD_MULTIPLY(a->vD.y, b->vD.w);
    dest->vD.z = RD_MULTIPLY(a->vA.z, b->vD.x) + RD_MULTIPLY(a->vB.z, b->vD.y) + RD_MULTIPLY(a->vC.z, b->vD.z) + RD_MULTIPLY(a->vD.z, b->vD.w);
    dest->vD.w = RD_MULTIPLY(a->vA.w, b->vD.x) + RD_MULTIPLY(a->vB.w, b->vD.y) + RD_MULTIPLY(a->vC.w, b->vD.z) + RD_MULTIPLY(a->vD.w, b->vD.w);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTOS(rdMatrix_countfit_10, 30, 13) CF_PROTO(rdMatrix_countfit_23, 16) /* end count-fit */
void J3DAPI rdMatrix_PreMultiply34(rdMatrix34* a, const rdMatrix34* b)
{
    rdMatrix34 acpy;

    memcpy(&acpy, a, sizeof(rdMatrix34));

    a->rvec.x = RD_MULTIPLY(acpy.rvec.x, b->rvec.x) + RD_MULTIPLY(acpy.lvec.x, b->rvec.y) + RD_MULTIPLY(acpy.uvec.x, b->rvec.z);
    a->rvec.y = RD_MULTIPLY(acpy.rvec.y, b->rvec.x) + RD_MULTIPLY(acpy.lvec.y, b->rvec.y) + RD_MULTIPLY(acpy.uvec.y, b->rvec.z);
    a->rvec.z = RD_MULTIPLY(acpy.rvec.z, b->rvec.x) + RD_MULTIPLY(acpy.lvec.z, b->rvec.y) + RD_MULTIPLY(acpy.uvec.z, b->rvec.z);

    a->lvec.x = RD_MULTIPLY(acpy.rvec.x, b->lvec.x) + RD_MULTIPLY(acpy.lvec.x, b->lvec.y) + RD_MULTIPLY(acpy.uvec.x, b->lvec.z);
    a->lvec.y = RD_MULTIPLY(acpy.rvec.y, b->lvec.x) + RD_MULTIPLY(acpy.lvec.y, b->lvec.y) + RD_MULTIPLY(acpy.uvec.y, b->lvec.z);
    a->lvec.z = RD_MULTIPLY(acpy.rvec.z, b->lvec.x) + RD_MULTIPLY(acpy.lvec.z, b->lvec.y) + RD_MULTIPLY(acpy.uvec.z, b->lvec.z);

    a->uvec.x = RD_MULTIPLY(acpy.rvec.x, b->uvec.x) + RD_MULTIPLY(acpy.lvec.x, b->uvec.y) + RD_MULTIPLY(acpy.uvec.x, b->uvec.z);
    a->uvec.y = RD_MULTIPLY(acpy.rvec.y, b->uvec.x) + RD_MULTIPLY(acpy.lvec.y, b->uvec.y) + RD_MULTIPLY(acpy.uvec.y, b->uvec.z);
    a->uvec.z = RD_MULTIPLY(acpy.rvec.z, b->uvec.x) + RD_MULTIPLY(acpy.lvec.z, b->uvec.y) + RD_MULTIPLY(acpy.uvec.z, b->uvec.z);

    a->dvec.x = RD_MULTIPLY(acpy.rvec.x, b->dvec.x) + RD_MULTIPLY(acpy.lvec.x, b->dvec.y) + RD_MULTIPLY(acpy.uvec.x, b->dvec.z) + acpy.dvec.x;
    a->dvec.y = RD_MULTIPLY(acpy.rvec.y, b->dvec.x) + RD_MULTIPLY(acpy.lvec.y, b->dvec.y) + RD_MULTIPLY(acpy.uvec.y, b->dvec.z) + acpy.dvec.y;
    a->dvec.z = RD_MULTIPLY(acpy.rvec.z, b->dvec.x) + RD_MULTIPLY(acpy.lvec.z, b->dvec.y) + RD_MULTIPLY(acpy.uvec.z, b->dvec.z) + acpy.dvec.z;
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PreMultiply44(rdMatrix44* a, const rdMatrix44* b)
{
    rdMatrix44 acpy;

    memcpy(&acpy, a, sizeof(rdMatrix44));

    a->vA.x = RD_MULTIPLY(acpy.vA.x, b->vA.x) + RD_MULTIPLY(acpy.vB.x, b->vA.y) + RD_MULTIPLY(acpy.vC.x, b->vA.z) + RD_MULTIPLY(acpy.vD.x, b->vA.w);
    a->vA.y = RD_MULTIPLY(acpy.vA.y, b->vA.x) + RD_MULTIPLY(acpy.vB.y, b->vA.y) + RD_MULTIPLY(acpy.vC.y, b->vA.z) + RD_MULTIPLY(acpy.vD.y, b->vA.w);
    a->vA.z = RD_MULTIPLY(acpy.vA.z, b->vA.x) + RD_MULTIPLY(acpy.vB.z, b->vA.y) + RD_MULTIPLY(acpy.vC.z, b->vA.z) + RD_MULTIPLY(acpy.vD.z, b->vA.w);
    a->vA.w = RD_MULTIPLY(acpy.vA.w, b->vA.x) + RD_MULTIPLY(acpy.vB.w, b->vA.y) + RD_MULTIPLY(acpy.vC.w, b->vA.z) + RD_MULTIPLY(acpy.vD.w, b->vA.w);

    a->vB.x = RD_MULTIPLY(acpy.vA.x, b->vB.x) + RD_MULTIPLY(acpy.vB.x, b->vB.y) + RD_MULTIPLY(acpy.vC.x, b->vB.z) + RD_MULTIPLY(acpy.vD.x, b->vB.w);
    a->vB.y = RD_MULTIPLY(acpy.vA.y, b->vB.x) + RD_MULTIPLY(acpy.vB.y, b->vB.y) + RD_MULTIPLY(acpy.vC.y, b->vB.z) + RD_MULTIPLY(acpy.vD.y, b->vB.w);
    a->vB.z = RD_MULTIPLY(acpy.vA.z, b->vB.x) + RD_MULTIPLY(acpy.vB.z, b->vB.y) + RD_MULTIPLY(acpy.vC.z, b->vB.z) + RD_MULTIPLY(acpy.vD.z, b->vB.w);
    a->vB.w = RD_MULTIPLY(acpy.vA.w, b->vB.x) + RD_MULTIPLY(acpy.vB.w, b->vB.y) + RD_MULTIPLY(acpy.vC.w, b->vB.z) + RD_MULTIPLY(acpy.vD.w, b->vB.w);

    a->vC.x = RD_MULTIPLY(acpy.vA.x, b->vC.x) + RD_MULTIPLY(acpy.vB.x, b->vC.y) + RD_MULTIPLY(acpy.vC.x, b->vC.z) + RD_MULTIPLY(acpy.vD.x, b->vC.w);
    a->vC.y = RD_MULTIPLY(acpy.vA.y, b->vC.x) + RD_MULTIPLY(acpy.vB.y, b->vC.y) + RD_MULTIPLY(acpy.vC.y, b->vC.z) + RD_MULTIPLY(acpy.vD.y, b->vC.w);
    a->vC.z = RD_MULTIPLY(acpy.vA.z, b->vC.x) + RD_MULTIPLY(acpy.vB.z, b->vC.y) + RD_MULTIPLY(acpy.vC.z, b->vC.z) + RD_MULTIPLY(acpy.vD.z, b->vC.w);
    a->vC.w = RD_MULTIPLY(acpy.vA.w, b->vC.x) + RD_MULTIPLY(acpy.vB.w, b->vC.y) + RD_MULTIPLY(acpy.vC.w, b->vC.z) + RD_MULTIPLY(acpy.vD.w, b->vC.w);

    a->vD.x = RD_MULTIPLY(acpy.vA.x, b->vD.x) + RD_MULTIPLY(acpy.vB.x, b->vD.y) + RD_MULTIPLY(acpy.vC.x, b->vD.z) + RD_MULTIPLY(acpy.vD.x, b->vD.w);
    a->vD.y = RD_MULTIPLY(acpy.vA.y, b->vD.x) + RD_MULTIPLY(acpy.vB.y, b->vD.y) + RD_MULTIPLY(acpy.vC.y, b->vD.z) + RD_MULTIPLY(acpy.vD.y, b->vD.w);
    a->vD.z = RD_MULTIPLY(acpy.vA.z, b->vD.x) + RD_MULTIPLY(acpy.vB.z, b->vD.y) + RD_MULTIPLY(acpy.vC.z, b->vD.z) + RD_MULTIPLY(acpy.vD.z, b->vD.w);
    a->vD.w = RD_MULTIPLY(acpy.vA.w, b->vD.x) + RD_MULTIPLY(acpy.vB.w, b->vD.y) + RD_MULTIPLY(acpy.vC.w, b->vD.z) + RD_MULTIPLY(acpy.vD.w, b->vD.w);
}

void J3DAPI rdMatrix_PostMultiply34(rdMatrix34* a, const rdMatrix34* b)
{
    rdMatrix34 acpy;

    memcpy(&acpy, a, sizeof(rdMatrix34));

    a->rvec.x = RD_MULTIPLY(b->rvec.x, acpy.rvec.x) + RD_MULTIPLY(b->lvec.x, acpy.rvec.y) + RD_MULTIPLY(b->uvec.x, acpy.rvec.z);
    a->rvec.y = RD_MULTIPLY(b->rvec.y, acpy.rvec.x) + RD_MULTIPLY(b->lvec.y, acpy.rvec.y) + RD_MULTIPLY(b->uvec.y, acpy.rvec.z);
    a->rvec.z = RD_MULTIPLY(b->rvec.z, acpy.rvec.x) + RD_MULTIPLY(b->lvec.z, acpy.rvec.y) + RD_MULTIPLY(b->uvec.z, acpy.rvec.z);

    a->lvec.x = RD_MULTIPLY(b->rvec.x, acpy.lvec.x) + RD_MULTIPLY(b->lvec.x, acpy.lvec.y) + RD_MULTIPLY(b->uvec.x, acpy.lvec.z);
    a->lvec.y = RD_MULTIPLY(b->rvec.y, acpy.lvec.x) + RD_MULTIPLY(b->lvec.y, acpy.lvec.y) + RD_MULTIPLY(b->uvec.y, acpy.lvec.z);
    a->lvec.z = RD_MULTIPLY(b->rvec.z, acpy.lvec.x) + RD_MULTIPLY(b->lvec.z, acpy.lvec.y) + RD_MULTIPLY(b->uvec.z, acpy.lvec.z);

    a->uvec.x = RD_MULTIPLY(b->rvec.x, acpy.uvec.x) + RD_MULTIPLY(b->lvec.x, acpy.uvec.y) + RD_MULTIPLY(b->uvec.x, acpy.uvec.z);
    a->uvec.y = RD_MULTIPLY(b->rvec.y, acpy.uvec.x) + RD_MULTIPLY(b->lvec.y, acpy.uvec.y) + RD_MULTIPLY(b->uvec.y, acpy.uvec.z);
    a->uvec.z = RD_MULTIPLY(b->rvec.z, acpy.uvec.x) + RD_MULTIPLY(b->lvec.z, acpy.uvec.y) + RD_MULTIPLY(b->uvec.z, acpy.uvec.z);

    a->dvec.x = RD_MULTIPLY(b->rvec.x, acpy.dvec.x) + RD_MULTIPLY(b->lvec.x, acpy.dvec.y) + RD_MULTIPLY(b->uvec.x, acpy.dvec.z) + b->dvec.x;
    a->dvec.y = RD_MULTIPLY(b->rvec.y, acpy.dvec.x) + RD_MULTIPLY(b->lvec.y, acpy.dvec.y) + RD_MULTIPLY(b->uvec.y, acpy.dvec.z) + b->dvec.y;
    a->dvec.z = RD_MULTIPLY(b->rvec.z, acpy.dvec.x) + RD_MULTIPLY(b->lvec.z, acpy.dvec.y) + RD_MULTIPLY(b->uvec.z, acpy.dvec.z) + b->dvec.z;
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PostMultiply44(rdMatrix44* a, const rdMatrix44* b)
{
    rdMatrix44 acpy;

    memcpy(&acpy, a, sizeof(rdMatrix44));

    a->vA.x = RD_MULTIPLY(b->vA.x, acpy.vA.x) + RD_MULTIPLY(b->vB.x, acpy.vA.y) + RD_MULTIPLY(b->vC.x, acpy.vA.z) + RD_MULTIPLY(b->vD.x, acpy.vA.w);
    a->vA.y = RD_MULTIPLY(b->vA.y, acpy.vA.x) + RD_MULTIPLY(b->vB.y, acpy.vA.y) + RD_MULTIPLY(b->vC.y, acpy.vA.z) + RD_MULTIPLY(b->vD.y, acpy.vA.w);
    a->vA.z = RD_MULTIPLY(b->vA.z, acpy.vA.x) + RD_MULTIPLY(b->vB.z, acpy.vA.y) + RD_MULTIPLY(b->vC.z, acpy.vA.z) + RD_MULTIPLY(b->vD.z, acpy.vA.w);
    a->vA.w = RD_MULTIPLY(b->vA.w, acpy.vA.x) + RD_MULTIPLY(b->vB.w, acpy.vA.y) + RD_MULTIPLY(b->vC.w, acpy.vA.z) + RD_MULTIPLY(b->vD.w, acpy.vA.w);

    a->vB.x = RD_MULTIPLY(b->vA.x, acpy.vB.x) + RD_MULTIPLY(b->vB.x, acpy.vB.y) + RD_MULTIPLY(b->vC.x, acpy.vB.z) + RD_MULTIPLY(b->vD.x, acpy.vB.w);
    a->vB.y = RD_MULTIPLY(b->vA.y, acpy.vB.x) + RD_MULTIPLY(b->vB.y, acpy.vB.y) + RD_MULTIPLY(b->vC.y, acpy.vB.z) + RD_MULTIPLY(b->vD.y, acpy.vB.w);
    a->vB.z = RD_MULTIPLY(b->vA.z, acpy.vB.x) + RD_MULTIPLY(b->vB.z, acpy.vB.y) + RD_MULTIPLY(b->vC.z, acpy.vB.z) + RD_MULTIPLY(b->vD.z, acpy.vB.w);
    a->vB.w = RD_MULTIPLY(b->vA.w, acpy.vB.x) + RD_MULTIPLY(b->vB.w, acpy.vB.y) + RD_MULTIPLY(b->vC.w, acpy.vB.z) + RD_MULTIPLY(b->vD.w, acpy.vB.w);

    a->vC.x = RD_MULTIPLY(b->vA.x, acpy.vC.x) + RD_MULTIPLY(b->vB.x, acpy.vC.y) + RD_MULTIPLY(b->vC.x, acpy.vC.z) + RD_MULTIPLY(b->vD.x, acpy.vC.w);
    a->vC.y = RD_MULTIPLY(b->vA.y, acpy.vC.x) + RD_MULTIPLY(b->vB.y, acpy.vC.y) + RD_MULTIPLY(b->vC.y, acpy.vC.z) + RD_MULTIPLY(b->vD.y, acpy.vC.w);
    a->vC.z = RD_MULTIPLY(b->vA.z, acpy.vC.x) + RD_MULTIPLY(b->vB.z, acpy.vC.y) + RD_MULTIPLY(b->vC.z, acpy.vC.z) + RD_MULTIPLY(b->vD.z, acpy.vC.w);
    a->vC.w = RD_MULTIPLY(b->vA.w, acpy.vC.x) + RD_MULTIPLY(b->vB.w, acpy.vC.y) + RD_MULTIPLY(b->vC.w, acpy.vC.z) + RD_MULTIPLY(b->vD.w, acpy.vC.w);

    a->vD.x = RD_MULTIPLY(b->vA.x, acpy.vD.x) + RD_MULTIPLY(b->vB.x, acpy.vD.y) + RD_MULTIPLY(b->vC.x, acpy.vD.z) + RD_MULTIPLY(b->vD.x, acpy.vD.w);
    a->vD.y = RD_MULTIPLY(b->vA.y, acpy.vD.x) + RD_MULTIPLY(b->vB.y, acpy.vD.y) + RD_MULTIPLY(b->vC.y, acpy.vD.z) + RD_MULTIPLY(b->vD.y, acpy.vD.w);
    a->vD.z = RD_MULTIPLY(b->vA.z, acpy.vD.x) + RD_MULTIPLY(b->vB.z, acpy.vD.y) + RD_MULTIPLY(b->vC.z, acpy.vD.z) + RD_MULTIPLY(b->vD.z, acpy.vD.w);
    a->vD.w = RD_MULTIPLY(b->vA.w, acpy.vD.x) + RD_MULTIPLY(b->vB.w, acpy.vD.y) + RD_MULTIPLY(b->vC.w, acpy.vD.z) + RD_MULTIPLY(b->vD.w, acpy.vD.w);
}

void J3DAPI rdMatrix_PreRotate34(rdMatrix34* mat, const rdVector3* pyr)
{
    rdMatrix34 rotMat;
    rdMatrix_BuildRotate34(&rotMat, pyr);
    rdMatrix_PreMultiply34(mat, &rotMat);
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PreRotate44(rdMatrix44* mat, const rdVector3* pyr)
{
    rdMatrix44 tmpMat;

    rdMatrix_BuildRotate44(&tmpMat, pyr);
    rdMatrix_PreMultiply44(mat, &tmpMat);
}

void J3DAPI rdMatrix_PostRotate34(rdMatrix34* mat, const rdVector3* vecPYR)
{
    rdMatrix34 rotMat;
    rdMatrix_BuildRotate34(&rotMat, vecPYR);
    rdMatrix_PostMultiply34(mat, &rotMat);
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PostRotate44(rdMatrix44* mat, const rdVector3* pyr)
{
    rdMatrix44 tmpMat;

    rdMatrix_BuildRotate44(&tmpMat, pyr);
    rdMatrix_PostMultiply44(mat, &tmpMat);
}

void J3DAPI rdMatrix_PreTranslate34(rdMatrix34* mat, const rdVector3* vec)
{
    rdMatrix34 tmat;
    rdMatrix_BuildTranslate34(&tmat, vec);
    rdMatrix_PreMultiply34(mat, &tmat);
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PreTranslate44(rdMatrix44* mat, const rdVector3* vec)
{
    rdMatrix44 tmpMat;

    rdMatrix_BuildTranslate44(&tmpMat, vec);
    rdMatrix_PreMultiply44(mat, &tmpMat);
}

void J3DAPI rdMatrix_PostTranslate34(rdMatrix34* pMat, const rdVector3* pVec)
{
    pMat->dvec.x += pVec->x;
    pMat->dvec.y += pVec->y;
    pMat->dvec.z += pVec->z;
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PostTranslate44(rdMatrix44* mat, const rdVector3* vec)
{
    mat->vD.x += vec->x;
    mat->vD.y += vec->y;
    mat->vD.z += vec->z;
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PreScale34(rdMatrix34* mat, const rdVector3* vec)
{
    rdMatrix34 tmpMat;

    rdMatrix_BuildScale34(&tmpMat, vec);
    rdMatrix_PreMultiply34(mat, &tmpMat);
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PreScale44(rdMatrix44* mat, const rdVector4* vec)
{
    rdMatrix44 tmpMat;

    rdMatrix_BuildScale44(&tmpMat, (const rdVector3*)vec);
    rdMatrix_PreMultiply44(mat, &tmpMat);
}

void J3DAPI rdMatrix_PostScale34(rdMatrix34* mat, const rdVector3* vec)
{
    rdMatrix34 tmpMat;

    rdMatrix_BuildScale34(&tmpMat, vec);
    rdMatrix_PostMultiply34(mat, &tmpMat);
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_PostScale44(rdMatrix44* mat, const rdVector4* vec)
{
    rdMatrix44 tmpMat;

    rdMatrix_BuildScale44(&tmpMat, (const rdVector3*)vec);
    rdMatrix_PostMultiply44(mat, &tmpMat);
}

void J3DAPI rdMatrix_SetRowVector34(rdMatrix34* mat, int row, const rdVector3* vec)
{
    rdVector_Copy3(&((rdVector3*)mat)[row], vec);
}

void J3DAPI rdMatrix_SetRowVector44(rdMatrix44* mat, int row, const rdVector4* vec)
{
    rdVector_Copy4(&((rdVector4*)mat)[row], vec);
}

void J3DAPI rdMatrix_GetRowVector34(rdMatrix34* mat, int row, rdVector3* vec)
{
    rdVector_Copy3(vec, &((rdVector3*)mat)[row]);
}

void J3DAPI rdMatrix_GetRowVector44(rdMatrix44* mat, int row, rdVector4* vec)
{
    rdVector_Copy3((rdVector3*)vec, (rdVector3*)&((rdVector4*)mat)[row]);
}
 /* count-fit: placeholder declarations, not reconstructed source */ CF_PROTO(rdMatrix_countfit_30, 30) CF_PROTO(rdMatrix_countfit_31, 0) /* end count-fit */
void J3DAPI rdMatrix_TransformVector34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat)
{
#line 1611
    RD_ASSERTREL(dest != src);
    dest->x = RD_MULTIPLY(mat->rvec.x, src->x) + RD_MULTIPLY(mat->lvec.x, src->y) + RD_MULTIPLY(mat->uvec.x, src->z);
    dest->y = RD_MULTIPLY(mat->rvec.y, src->x) + RD_MULTIPLY(mat->lvec.y, src->y) + RD_MULTIPLY(mat->uvec.y, src->z);
    dest->z = RD_MULTIPLY(mat->rvec.z, src->x) + RD_MULTIPLY(mat->lvec.z, src->y) + RD_MULTIPLY(mat->uvec.z, src->z);
}

void J3DAPI rdMatrix_TransformVectorOrtho34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat)
{
#line 1642
    RD_ASSERTREL(dest != src);
    dest->x = rdVector_Dot3(&mat->rvec, src);
    dest->y = rdVector_Dot3(&mat->lvec, src);
    dest->z = rdVector_Dot3(&mat->uvec, src);
}

void J3DAPI rdMatrix_TransformVector34Acc(rdVector3* dest, const rdMatrix34* mat)
{
    rdVector3 vec;
    vec.x = RD_MULTIPLY(mat->rvec.x, dest->x) + RD_MULTIPLY(mat->lvec.x, dest->y) + RD_MULTIPLY(mat->uvec.x, dest->z);
    vec.y = RD_MULTIPLY(mat->rvec.y, dest->x) + RD_MULTIPLY(mat->lvec.y, dest->y) + RD_MULTIPLY(mat->uvec.y, dest->z);
    vec.z = RD_MULTIPLY(mat->rvec.z, dest->x) + RD_MULTIPLY(mat->lvec.z, dest->y) + RD_MULTIPLY(mat->uvec.z, dest->z);
    rdVector_Copy3(dest, &vec);
}

/* Linker-discarded (no retail symbol); the body is the debug build's text. */
void J3DAPI rdMatrix_TransformVector44(rdVector4* dest, const rdVector4* src, const rdMatrix44* mat)
{
#line 1694
    RD_ASSERTREL(dest != src);
    dest->x = RD_MULTIPLY(mat->vA.x, src->x) + RD_MULTIPLY(mat->vB.x, src->y) + RD_MULTIPLY(mat->vC.x, src->z) + RD_MULTIPLY(mat->vD.x, src->w);
    dest->y = RD_MULTIPLY(mat->vA.y, src->x) + RD_MULTIPLY(mat->vB.y, src->y) + RD_MULTIPLY(mat->vC.y, src->z) + RD_MULTIPLY(mat->vD.y, src->w);
    dest->z = RD_MULTIPLY(mat->vA.z, src->x) + RD_MULTIPLY(mat->vB.z, src->y) + RD_MULTIPLY(mat->vC.z, src->z) + RD_MULTIPLY(mat->vD.z, src->w);
    dest->w = RD_MULTIPLY(mat->vA.w, src->x) + RD_MULTIPLY(mat->vB.w, src->y) + RD_MULTIPLY(mat->vC.w, src->z) + RD_MULTIPLY(mat->vD.w, src->w);
}

void J3DAPI rdMatrix_TransformVector44Acc(rdVector4* dest, const rdMatrix44* mat)
{
    rdVector4 vec;

    vec.x = RD_MULTIPLY(mat->vA.x, dest->x) + RD_MULTIPLY(mat->vB.x, dest->y) + RD_MULTIPLY(mat->vC.x, dest->z) + RD_MULTIPLY(mat->vD.x, dest->w);
    vec.y = RD_MULTIPLY(mat->vA.y, dest->x) + RD_MULTIPLY(mat->vB.y, dest->y) + RD_MULTIPLY(mat->vC.y, dest->z) + RD_MULTIPLY(mat->vD.y, dest->w);
    vec.z = RD_MULTIPLY(mat->vA.z, dest->x) + RD_MULTIPLY(mat->vB.z, dest->y) + RD_MULTIPLY(mat->vC.z, dest->z) + RD_MULTIPLY(mat->vD.z, dest->w);
    vec.w = RD_MULTIPLY(mat->vA.w, dest->x) + RD_MULTIPLY(mat->vB.w, dest->y) + RD_MULTIPLY(mat->vC.w, dest->z) + RD_MULTIPLY(mat->vD.w, dest->w);
    rdVector_Copy4(dest, &vec);
}

void J3DAPI rdMatrix_TransformPoint34(rdVector3* dest, const rdVector3* src, const rdMatrix34* mat)
{
#line 1749
    RD_ASSERTREL(dest != src);
    dest->x = RD_MULTIPLY(mat->rvec.x, src->x) + RD_MULTIPLY(mat->lvec.x, src->y) + RD_MULTIPLY(mat->uvec.x, src->z) + mat->dvec.x;
    dest->y = RD_MULTIPLY(mat->rvec.y, src->x) + RD_MULTIPLY(mat->lvec.y, src->y) + RD_MULTIPLY(mat->uvec.y, src->z) + mat->dvec.y;
    dest->z = RD_MULTIPLY(mat->rvec.z, src->x) + RD_MULTIPLY(mat->lvec.z, src->y) + RD_MULTIPLY(mat->uvec.z, src->z) + mat->dvec.z;
}

void J3DAPI rdMatrix_TransformPoint34Acc(rdVector3* dest, const rdMatrix34* mat)
{
    rdVector3 point;
    point.x = RD_MULTIPLY(mat->rvec.x, dest->x) + RD_MULTIPLY(mat->lvec.x, dest->y) + RD_MULTIPLY(mat->uvec.x, dest->z) + mat->dvec.x;
    point.y = RD_MULTIPLY(mat->rvec.y, dest->x) + RD_MULTIPLY(mat->lvec.y, dest->y) + RD_MULTIPLY(mat->uvec.y, dest->z) + mat->dvec.y;
    point.z = RD_MULTIPLY(mat->rvec.z, dest->x) + RD_MULTIPLY(mat->lvec.z, dest->y) + RD_MULTIPLY(mat->uvec.z, dest->z) + mat->dvec.z;
    rdVector_Copy3(dest, &point);
}

// Linker-discarded; body from the debug build (0x50b832, `__LINE__` 1801). Retail's w row adds vD.z, not vD.w (original bug).
void J3DAPI rdMatrix_TransformPoint44(rdVector4* dest, const rdVector4* src, const rdMatrix44* mat)
{
#line 1801
    RD_ASSERTREL(dest != src);
    dest->x = RD_MULTIPLY(mat->vA.x, src->x) + RD_MULTIPLY(mat->vB.x, src->y) + RD_MULTIPLY(mat->vC.x, src->z) + mat->vD.x;
    dest->y = RD_MULTIPLY(mat->vA.y, src->x) + RD_MULTIPLY(mat->vB.y, src->y) + RD_MULTIPLY(mat->vC.y, src->z) + mat->vD.y;
    dest->z = RD_MULTIPLY(mat->vA.z, src->x) + RD_MULTIPLY(mat->vB.z, src->y) + RD_MULTIPLY(mat->vC.z, src->z) + mat->vD.z;
    dest->w = RD_MULTIPLY(mat->vA.w, src->x) + RD_MULTIPLY(mat->vB.w, src->y) + RD_MULTIPLY(mat->vC.w, src->z) + mat->vD.z;
}

// Linker-discarded; body from the debug build (0x50b924); the same vD.z in the w row.
void J3DAPI rdMatrix_TransformPoint44Acc(rdVector4* dest, const rdMatrix44* mat)
{
    rdVector4 point;

    point.x = RD_MULTIPLY(mat->vA.x, dest->x) + RD_MULTIPLY(mat->vB.x, dest->y) + RD_MULTIPLY(mat->vC.x, dest->z) + mat->vD.x;
    point.y = RD_MULTIPLY(mat->vA.y, dest->x) + RD_MULTIPLY(mat->vB.y, dest->y) + RD_MULTIPLY(mat->vC.y, dest->z) + mat->vD.y;
    point.z = RD_MULTIPLY(mat->vA.z, dest->x) + RD_MULTIPLY(mat->vB.z, dest->y) + RD_MULTIPLY(mat->vC.z, dest->z) + mat->vD.z;
    point.w = RD_MULTIPLY(mat->vA.w, dest->x) + RD_MULTIPLY(mat->vB.w, dest->y) + RD_MULTIPLY(mat->vC.w, dest->z) + mat->vD.z;
    rdVector_Copy4(dest, &point);
}

void J3DAPI rdMatrix_TransformPointList34(const rdMatrix34* mat, const rdVector3* aSrc, rdVector3* aDest, size_t size)
{
    while ( size-- )
    {
        aDest->x = RD_MULTIPLY(mat->rvec.x, aSrc->x) + RD_MULTIPLY(mat->lvec.x, aSrc->y) + RD_MULTIPLY(mat->uvec.x, aSrc->z) + mat->dvec.x;
        aDest->y = RD_MULTIPLY(mat->rvec.y, aSrc->x) + RD_MULTIPLY(mat->lvec.y, aSrc->y) + RD_MULTIPLY(mat->uvec.y, aSrc->z) + mat->dvec.y;
        aDest->z = RD_MULTIPLY(mat->rvec.z, aSrc->x) + RD_MULTIPLY(mat->lvec.z, aSrc->y) + RD_MULTIPLY(mat->uvec.z, aSrc->z) + mat->dvec.z;
        ++aSrc;
        ++aDest;
    }
}

// Linker-discarded; body from the debug build (0x50bac6): x, y and z only.
void J3DAPI rdMatrix_TransformPointList44(const rdMatrix44* mat, const rdVector4* aSrc, rdVector4* aDest, size_t size)
{
    while ( size-- )
    {
        aDest->x = RD_MULTIPLY(mat->vA.x, aSrc->x) + RD_MULTIPLY(mat->vB.x, aSrc->y) + RD_MULTIPLY(mat->vC.x, aSrc->z) + mat->vD.x;
        aDest->y = RD_MULTIPLY(mat->vA.y, aSrc->x) + RD_MULTIPLY(mat->vB.y, aSrc->y) + RD_MULTIPLY(mat->vC.y, aSrc->z) + mat->vD.y;
        aDest->z = RD_MULTIPLY(mat->vA.z, aSrc->x) + RD_MULTIPLY(mat->vB.z, aSrc->y) + RD_MULTIPLY(mat->vC.z, aSrc->z) + mat->vD.z;
        ++aSrc;
        ++aDest;
    }
}
