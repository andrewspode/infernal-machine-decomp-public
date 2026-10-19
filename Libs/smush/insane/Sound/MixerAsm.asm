; MixerAsm.asm: INSANE's sound-mixer buffer helpers (retail 0x004EC560-0x004EC720), assembled with MASM 6.13.
;
; Eight cdecl procedures that copy, fill and table-convert sample buffers. Engine.c calls mixer_MemCopy and
; mixer_MemFill (0x004E746F, 0x004E748F); nothing in the image calls the other six. Every procedure keeps
; the DOS-era frame of the INSANE assembler sources: it saves es and points it at ds (push ds; pop es) before
; the string instructions. The 16-bit segment _TEXT16 (retail PE section _TEXT16, 0x00505000) is also MASM
; output; nothing references it.
;
; Arguments: pDst [ebp+8], pSrc or value [ebp+12], n [ebp+16]; the table procedures also take
; pTables [ebp+20] and the table index [ebp+24] (512-byte word tables, 256-byte byte tables).

	.386
	.model flat, c

	.code

mixer_MemCopy PROC
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
	or	ecx,ecx
	je	L4EC587
	test	ecx,3
	je	L4EC58D
	shr	ecx,1
	rep movsw
	adc	ecx,ecx
	rep movsb
L4EC587:
	pop	ecx
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	ret
L4EC58D:
	shr	ecx,2
	rep movsd
	pop	ecx
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	ret
mixer_MemCopy ENDP

mixer_MemFill PROC
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
	jl	L4EC5D5
L4EC5BE:
	push	ecx
	mov	ecx,1388h
	rep stosd
	pop	ecx
	sub	ecx,4E20h
	cmp	ecx,4E20h
	jg	L4EC5BE
L4EC5D5:
	test	ecx,3
	je	L4EC5EC
	shr	ecx,1
	rep stosw
	adc	ecx,ecx
	rep stosb
	pop	ecx
	pop	es
	pop	edi
	pop	eax
	pop	ebp
	ret
L4EC5EC:
	shr	ecx,2
	rep stosd
	pop	ecx
	pop	es
	pop	edi
	pop	eax
	pop	ebp
	ret
mixer_MemFill ENDP

mixer_Lut16 PROC
	push	ebp
	mov	ebp,esp
	push	esi
	push	edi
	push	es
	push	ecx
	push	ebx
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebx,dword ptr [ebp+14h]
	mov	eax,dword ptr [ebp+18h]
	shl	eax,9
	add	ebx,eax
	xor	eax,eax
L4EC617:
	lodsb
	and	eax,0FFh
	mov	ax,word ptr [ebx+eax*2]
	stosw
	loop	L4EC617
	pop	ebx
	pop	ecx
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	ret
mixer_Lut16 ENDP

mixer_Lut16Add PROC
	push	ebp
	mov	ebp,esp
	push	esi
	push	edi
	push	es
	push	ecx
	push	ebx
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebx,dword ptr [ebp+14h]
	mov	eax,dword ptr [ebp+18h]
	shl	eax,9
	add	ebx,eax
	xor	eax,eax
L4EC64C:
	lodsb
	and	eax,0FFh
	mov	ax,word ptr [ebx+eax*2]
	add	ax,word ptr [edi]
	stosw
	loop	L4EC64C
	pop	ebx
	pop	ecx
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	ret
mixer_Lut16Add ENDP

mixer_Fill16 PROC
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
	rep stosw
	pop	ecx
	pop	es
	pop	edi
	pop	eax
	pop	ebp
	ret
mixer_Fill16 ENDP

mixer_Lut8 PROC
	push	ebp
	mov	ebp,esp
	push	esi
	push	edi
	push	es
	push	ecx
	push	ebx
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebx,dword ptr [ebp+14h]
	mov	eax,dword ptr [ebp+18h]
	shl	eax,8
	add	ebx,eax
	xor	eax,eax
L4EC69F:
	lodsb
	mov	al,byte ptr [ebx+eax]
	stosb
	loop	L4EC69F
	pop	ebx
	pop	ecx
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	ret
mixer_Lut8 ENDP

mixer_Lut8Add PROC
	push	ebp
	mov	ebp,esp
	push	esi
	push	edi
	push	es
	push	ecx
	push	ebx
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebx,dword ptr [ebp+14h]
	mov	eax,dword ptr [ebp+18h]
	shl	eax,8
	add	ebx,eax
	xor	eax,eax
L4EC6CD:
	lodsb
	mov	al,byte ptr [ebx+eax]
	add	al,byte ptr [edi]
	stosb
	loop	L4EC6CD
	pop	ebx
	pop	ecx
	pop	es
	pop	edi
	pop	esi
	pop	ebp
	ret
mixer_Lut8Add ENDP

mixer_FillB PROC
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
	test	ecx,3
	jne	L4EC708
	mov	ah,al
	shr	ecx,1
	rep stosw
	adc	ecx,ecx
	rep stosb
	pop	ecx
	pop	es
	pop	edi
	pop	eax
	pop	ebp
	ret
L4EC708:
	mov	ah,al
	shl	eax,8
	mov	al,ah
	shl	eax,8
	mov	al,ah
	shr	ecx,2
	rep stosd
	pop	ecx
	pop	es
	pop	edi
	pop	eax
	pop	ebp
	ret
mixer_FillB ENDP

_TEXT16	SEGMENT USE16 PUBLIC 'CODE'
	int	1Bh
	iret
	iret
_TEXT16	ENDS

	END
