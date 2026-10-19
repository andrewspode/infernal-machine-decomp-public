/*
 * Intel libm floor.c 1.4.7.1 (floor, ceil and rint): retail 0x004E2F30 (floor, 0xB0 bytes) and
 * 0x004E2FE0 (ceil, 0x20 bytes). Not Microsoft's and not the game's: retail linked Intel C/C++ 4.0's
 * libm in place of LIBC's floor/ceil. The file name and the function bodies are
 * inferences from the 99100 libm.lib member floor.obu (identical code); the algorithm is the
 * Sun Microsystems fdlibm floor (Sun's notice below).
 *
 * Compiled with Intel C/C++ 4.0 `-w -Zl -Di386 -QIfdiv- -Op` (floor.obu's own recorded build
 * line). floor and ceil are byte-identical to retail after relocation; the order of the .data1 constants differs.
 *
 * Retail's `.data` holds this object's data, so it is source, not an artefact (the evidence:
 * `0x0054BCD0` is the double 2**52 `L`, `0x0054BCD8` the ident string `@(#)floor.c\t1.4.7.1 ...`, then
 * modf's ident string and support.c's data, i.e. libm.lib's floor/modf/support members in link order).
 * The file's data definitions also make ICL emit `fld 1.0; fadd [x]` (retail's operand order) instead of
 * `fld [x]; fadd 1.0`.
 *
 * `rint` is the third function of floor.obu. LINK dropped it as unreferenced, but its literal packet
 * (`1.0`, the last of the pool at `0x01860118`, which no retained code reaches) stayed: that packet is the
 * evidence it existed (relocations.yml `discarded`). The body is 4.4BSD libm's rint (common_source/floor.c:
 * the NaN test, `one`, `s` and `t`), which compiles byte-identical to floor.obu's rint; retail has no rint
 * code, only the pool packet it owns. Intel's libm descends from the BSD one (support.c's `national`
 * data, the `-Di386`/`-DNATIONAL` build lines).
 */

/*
 * Notices for the third-party text this file reconstructs.
 *
 * rint (below) is the 4.4BSD libm rint (common_source/floor.c), whose notice is:
 *
 * Copyright (c) 1985, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * floor and ceil follow the Sun Microsystems fdlibm algorithm (via modf), whose notice is:
 *
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 */
double modf(double x, double* iptr);
double copysign(double x, double y);

static double L = 4503599627370496.0E0; /* 2**52 */
static char sccs_id[] = "@(#)floor.c\t1.4.7.1 08/28/98 14:05:29";

double floor(double x)
{
    double y;
    if (x == 0.0 || x == -0.0)
        return x;
    if (x < 0.0) {
        x = -x;
        y = modf(x, &x);
        if (y != 0.0)
            x += 1.0;
        x = -x;
    } else {
        modf(x, &x);
    }
    return x;
}

double ceil(double x)
{
    return -floor(-x);
}

double rint(double x)
{
    double s, t;
    const double one = 1.0;

    if (x != x) /* NaN */
        return (x);
    if (copysign(x, one) >= L) /* already an integer */
        return (x);
    s = copysign(L, x);
    t = x + s; /* x+s rounded to integer */
    return (t - s);
}
