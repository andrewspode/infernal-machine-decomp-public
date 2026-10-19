; MemFill.asm: INSANE's byte fill (retail 0x004F04F8-0x004F054F), assembled with MASM 6.13.
;
; blit_MemCopyB(pDst, value, size): replicates the low byte of value into a dword and stores size bytes,
; in 20000-byte rep stosd runs, then the size & 3 odd bytes, then whole dwords. Framestream.c's
; framestream_CopyB calls it (0x004EC54F).

	.386
	.model flat, c

	.code

blit_MemCopyB PROC
	push	ebp
	mov	ebp,esp
	push	eax
	push	edi
	push	es
	push	ecx
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	eax,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ah,al
	shl	eax,8
	mov	al,ah
	shl	eax,8
	mov	al,ah
	cmp	ecx,4E20h
	jl	L4F0535
L4F051E:
	push	ecx
	mov	ecx,1388h
	rep stosd
	pop	ecx
	sub	ecx,4E20h
	cmp	ecx,4E20h
	jg	L4F051E
L4F0535:
	test	ecx,3
	je	L4F0544
	push	ecx
	and	ecx,3
	rep stosb
	pop	ecx
L4F0544:
	shr	ecx,2
	rep stosd
	pop	ecx
	pop	es
	pop	edi
	pop	eax
	pop	ebp
	ret
blit_MemCopyB ENDP

	END
