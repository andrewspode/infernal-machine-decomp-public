/*
 * Intel libm modf (retail 0x004E3000, 0xB0 bytes, code 0xA6): a separate object from floor/ceil in
 * retail, with its own flags. Not Microsoft's and not the game's: retail linked Intel C/C++ 4.0's libm
 * in place of LIBC's floor/ceil/modf. The file name is an inference (libm's modf.c 1.3.7.1
 * from the 99100 libm.lib member modf.obu); the algorithm is Sun Microsystems' fdlibm modf, whose notice
 * is kept below.
 *
 * Compiled with Intel C/C++ 4.0 `-O2 -QxM -Qsfalign-` (as set in the build). Not byte-identical: 61 of 63
 * instructions align, same frame; retail keeps iptr in ebx (loaded early, three callee-saved pushes)
 * where ICL 99100 loads it late into eax: its profile-guided P2 pass sinks iptr's parameter copy to the
 * store, retail's copy stayed in the entry block (the setup or source
 * fact that keeps it there is not yet found). `j0 <= -1` (not `j0 < 0`) is what makes ICL lay the j0<0
 * block inline like retail. With libm's own flags (-w -Zl -Di386 -QIfdiv-, aligned prologue) this source
 * is byte-identical to 99100's own modf.obu.
 * Retail's .data holds this object's ident string (0x0054BD00), so sccs_id below is source
 * (libm's .data is in retail).
 */
/*
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 */
static char sccs_id[] = "@(#)modf.c\t1.3.7.1 08/28/98 14:06:01";
typedef unsigned int u32;
typedef union { double d; struct { u32 lo; int hi; } w; } dbl;

double modf(double x, double* iptr)
{
    dbl u;
    double ip, frac;
    int neg = 0;
    int j0, hi;

    if (x < 0.0) {
        x = -x;
        neg = 1;
    }
    u.d = x;
    hi = u.w.hi;
    j0 = ((hi & 0x7ff00000) >> 20) - 1023;
    if (j0 <= -1) {
        ip = 0.0;
        frac = x;
    } else if (j0 <= 20) {
        u.w.lo = 0;
        u.w.hi = hi & (-1 << (20 - j0));
        ip = u.d;
        frac = x - ip;
    } else if (j0 <= 52) {
        u.w.lo &= (-1 << (52 - j0));
        ip = u.d;
        frac = x - ip;
    } else {
        ip = x;
        frac = 0.0;
    }
    if (neg)
        ip = -ip;
    *iptr = ip;
    if (neg)
        frac = -frac;
    return frac;
}
