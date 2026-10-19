; MemCopy.asm: INSANE's block copy (retail 0x004F04D0-0x004F04F5), assembled with MASM 6.13.
;
; blit_MemCopy(pDst, pSrc, size): copies size bytes, the n & 3 odd bytes first, then whole dwords.
; Framestream.c's framestream_Copy calls it (0x004EC52F).

	.386
	.model flat, c

	.code

blit_MemCopy PROC
	push	ebp
	mov	ebp,esp
	push	esi
	push	edi
	push	es
	push	ecx
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	and	ecx,3
	rep movsb
	mov	ecx,dword ptr [ebp+10h]
	shr	ecx,2
	rep movsd
	pop	ecx
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	ret
blit_MemCopy ENDP

	END
