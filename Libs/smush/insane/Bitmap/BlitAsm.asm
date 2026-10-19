; BlitAsm.asm: INSANE's rectangle copy (retail 0x004F2220-0x004F22B7), assembled with MASM 6.13.
;
; blit_Masm(pSrc, pDst, srcX, srcY, dstX, dstY, width, height, srcPitch, dstPitch): copies a width x height
; rectangle row by row (rep movsd, then the width & 3 odd bytes). Blit.c's blit_Wrapper calls it (0x004F0582).
; The pitches multiply as 16-bit values (mul bx / mul dx).

	.386
	.model flat, c

	.code

blit_Masm PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	esi,dword ptr [ebp+8]
	mov	edi,dword ptr [ebp+0Ch]
	mov	ebx,dword ptr [ebp+28h]
	add	esi,dword ptr [ebp+10h]
	mov	eax,dword ptr [ebp+14h]
	xor	edx,edx
	mul	bx
	shl	edx,10h
	or	eax,edx
	add	esi,eax
	mov	edx,dword ptr [ebp+2Ch]
	push	edx
	add	edi,dword ptr [ebp+18h]
	mov	eax,dword ptr [ebp+1Ch]
	movzx	edx,dx
	mul	dx
	shl	edx,10h
	or	eax,edx
	add	edi,eax
	pop	edx
	mov	ecx,dword ptr [ebp+20h]
	mov	ebp,dword ptr [ebp+24h]
	sub	ebx,ecx
	sub	edx,ecx
	mov	eax,ecx
	and	eax,3
	je	L4F228A
	push	eax
	shr	ecx,2
	push	ecx
L4F2271:
	mov	ecx,dword ptr [esp]
	rep movsd
	mov	ecx,dword ptr [esp+4]
	rep movsb
	add	edi,edx
	add	esi,ebx
	dec	ebp
	jg	L4F2271
	pop	ecx
	pop	ecx
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
L4F228A:
	or	edx,edx
	je	L4F22A4
	shr	ecx,2
	push	ecx
L4F2292:
	mov	ecx,dword ptr [esp]
	rep movsd
	add	edi,edx
	add	esi,ebx
	dec	ebp
	jg	L4F2292
	pop	ecx
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
L4F22A4:
	shr	ecx,2
	mov	edx,ecx
L4F22A9:
	mov	ecx,edx
	rep movsd
	add	esi,ebx
	dec	ebp
	jg	L4F22A9
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
blit_Masm ENDP

	END
