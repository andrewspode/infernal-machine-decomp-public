; DecodeAsm.asm: INSANE's pixel-format converters (retail 0x004F0590-0x004F09BA), assembled with MASM 6.13.
;
; decode_Lut8/decode_Lut16(pDst, pSrc, n, dstSize, pTable) map 8- or 16-bit source pixels through a table to
; 1-, 2-, 3- or 4-byte destination pixels (dstSize); the other six convert n pixels between 32-bit RGBA and
; 555, 565 and byte-swapped 565 words, two pixels per iteration after an odd first one. Decode.c calls all
; eight (decode_Convert, decode_ToRGBA, decode_Begin).

	.386
	.model flat, c

	.code

decode_Lut8 PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebx,dword ptr [ebp+14h]
	mov	edx,dword ptr [ebp+18h]
	cmp	bl,1
	je	L4F05BD
	cmp	bl,2
	je	L4F05CF
	cmp	bl,3
	je	L4F0605
	cmp	bl,4
	je	L4F061D
	jmp	L4F062D
L4F05BD:
	xor	eax,eax
L4F05BF:
	mov	al,byte ptr [esi]
	mov	bl,byte ptr [eax+edx]
	inc	esi
	mov	byte ptr [edi],bl
	dec	ecx
	lea	edi,[edi+1]
	jg	L4F05BF
	jmp	L4F062D
L4F05CF:
	xor	eax,eax
	mov	ebp,ecx
	xor	ecx,ecx
	shr	ebp,1
	jae	L4F05E6
	mov	al,byte ptr [esi]
	mov	bx,word ptr [edx+eax*2]
	inc	esi
	mov	word ptr [edi],bx
	lea	edi,[edi+2]
L4F05E6:
	mov	cx,word ptr [esi]
	mov	al,ch
	mov	bx,word ptr [edx+eax*2]
	xor	ch,ch
	shl	ebx,10h
	mov	bx,word ptr [edx+ecx*2]
	lea	esi,[esi+2]
	mov	dword ptr [edi],ebx
	dec	ebp
	lea	edi,[edi+4]
	jg	L4F05E6
	jmp	L4F062D
L4F0605:
	xor	eax,eax
L4F0607:
	mov	al,byte ptr [esi]
	mov	ebx,eax
	add	eax,eax
	add	eax,ebx
	mov	ebx,dword ptr [eax+edx]
	inc	esi
	mov	dword ptr [edi],ebx
	dec	ecx
	lea	edi,[edi+3]
	jg	L4F0607
	jmp	L4F062D
L4F061D:
	xor	eax,eax
L4F061F:
	mov	al,byte ptr [esi]
	mov	ebx,dword ptr [edx+eax*4]
	inc	esi
	mov	dword ptr [edi],ebx
	dec	ecx
	lea	edi,[edi+4]
	jg	L4F061F
L4F062D:
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
decode_Lut8 ENDP

decode_Lut16 PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebx,dword ptr [ebp+14h]
	mov	edx,dword ptr [ebp+18h]
	cmp	bl,1
	je	L4F0662
	cmp	bl,2
	je	L4F0677
	cmp	bl,3
	je	L4F06B5
	cmp	bl,4
	je	L4F06D0
	jmp	L4F06E3
L4F0662:
	xor	eax,eax
L4F0664:
	mov	ax,word ptr [esi]
	mov	bl,byte ptr [eax+edx]
	lea	esi,[esi+2]
	mov	byte ptr [edi],bl
	dec	ecx
	lea	edi,[edi+1]
	jg	L4F0664
	jmp	L4F06E3
L4F0677:
	xor	eax,eax
	mov	ebp,ecx
	xor	ecx,ecx
	shr	ebp,1
	jae	L4F0691
	mov	ax,word ptr [esi]
	mov	bx,word ptr [edx+eax*2]
	lea	esi,[esi+2]
	mov	word ptr [edi],bx
	lea	edi,[edi+2]
L4F0691:
	mov	ecx,dword ptr [esi]
	mov	ax,cx
	lea	esi,[esi+4]
	mov	bx,word ptr [edx+eax*2]
	shr	ecx,10h
	shl	ebx,10h
	mov	bx,word ptr [edx+ecx*2]
	lea	edi,[edi+4]
	ror	ebx,10h
	dec	ebp
	mov	dword ptr [edi-4],ebx
	jg	L4F0691
	jmp	L4F06E3
L4F06B5:
	xor	eax,eax
L4F06B7:
	mov	ax,word ptr [esi]
	mov	ebx,eax
	add	eax,eax
	add	eax,ebx
	mov	ebx,dword ptr [eax+edx]
	lea	esi,[esi+2]
	mov	dword ptr [edi],ebx
	dec	ecx
	lea	edi,[edi+3]
	jg	L4F06B7
	jmp	L4F06E3
L4F06D0:
	xor	eax,eax
L4F06D2:
	mov	ax,word ptr [esi]
	mov	ebx,dword ptr [edx+eax*4]
	lea	esi,[esi+2]
	mov	dword ptr [edi],ebx
	dec	ecx
	lea	edi,[edi+4]
	jg	L4F06D2
L4F06E3:
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
decode_Lut16 ENDP

decode_RGBAto555 PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebp,ecx
	shr	ebp,1
	jae	L4F071E
	mov	eax,dword ptr [esi]
	and	eax,0F8F8F8h
	mov	bl,ah
	mov	bh,al
	shr	bh,3
	shl	ebx,2
	shr	eax,0Bh
	or	bl,ah
	mov	word ptr [edi],bx
	add	esi,4
	add	edi,2
L4F071E:
	mov	eax,dword ptr [esi+4]
	mov	ecx,dword ptr [esi]
	and	eax,0F8F8F8h
	and	ecx,0F8F8F8h
	mov	bl,ah
	mov	dl,ch
	mov	bh,al
	mov	dh,cl
	shr	bh,3
	shr	dh,3
	shl	ebx,2
	shl	edx,2
	shr	eax,0Bh
	shr	ecx,0Bh
	or	bl,ah
	or	dl,ch
	shl	ebx,10h
	mov	bx,dx
	mov	dword ptr [edi],ebx
	add	esi,8
	add	edi,4
	dec	ebp
	jg	L4F071E
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
decode_RGBAto555 ENDP

decode_555toRGBA PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebp,ecx
	shr	ebp,1
	jae	L4F079C
	movzx	eax,word ptr [esi]
	mov	bl,ah
	shl	eax,3
	ror	ebx,7
	mov	bh,al
	shl	eax,3
	mov	bl,ah
	rol	ebx,8
	or	ebx,0FF070707h
	mov	dword ptr [edi],ebx
	add	esi,2
	add	edi,4
L4F079C:
	movzx	eax,word ptr [esi+2]
	movzx	ecx,word ptr [esi]
	mov	bl,ah
	mov	dl,ch
	shl	eax,3
	shl	ecx,3
	ror	ebx,7
	ror	edx,7
	mov	bh,al
	mov	dh,cl
	shl	eax,3
	shl	ecx,3
	mov	bl,ah
	mov	dl,ch
	rol	ebx,8
	rol	edx,8
	or	ebx,0FF070707h
	or	edx,0FF070707h
	mov	dword ptr [edi+4],ebx
	mov	dword ptr [edi],edx
	add	esi,4
	add	edi,8
	dec	ebp
	jg	L4F079C
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
decode_555toRGBA ENDP

decode_RGBAto565 PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebp,ecx
	shr	ebp,1
	jae	L4F081C
	mov	eax,dword ptr [esi]
	and	eax,0F8FCF8h
	mov	bl,ah
	mov	bh,al
	shr	bh,3
	shl	ebx,3
	shr	eax,0Bh
	or	bl,ah
	mov	word ptr [edi],bx
	add	esi,4
	add	edi,2
L4F081C:
	mov	eax,dword ptr [esi+4]
	mov	ecx,dword ptr [esi]
	and	eax,0F8FCF8h
	and	ecx,0F8FCF8h
	mov	bl,ah
	mov	dl,ch
	mov	bh,al
	mov	dh,cl
	shr	bh,3
	shr	dh,3
	shl	ebx,3
	shl	edx,3
	shr	eax,0Bh
	shr	ecx,0Bh
	or	bl,ah
	or	dl,ch
	shl	ebx,10h
	mov	bx,dx
	mov	dword ptr [edi],ebx
	add	esi,8
	add	edi,4
	dec	ebp
	jg	L4F081C
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
decode_RGBAto565 ENDP

decode_565toRGBA PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebp,ecx
	shr	ebp,1
	jae	L4F089A
	movzx	eax,word ptr [esi]
	mov	bl,ah
	shl	eax,3
	ror	ebx,8
	mov	bh,al
	shl	eax,2
	mov	bl,ah
	rol	ebx,8
	or	ebx,0FF070307h
	mov	dword ptr [edi],ebx
	add	esi,2
	add	edi,4
L4F089A:
	movzx	eax,word ptr [esi+2]
	movzx	ecx,word ptr [esi]
	mov	bl,ah
	mov	dl,ch
	shl	eax,3
	shl	ecx,3
	ror	ebx,8
	ror	edx,8
	mov	bh,al
	mov	dh,cl
	shl	eax,2
	shl	ecx,2
	mov	bl,ah
	mov	dl,ch
	rol	ebx,8
	rol	edx,8
	or	ebx,0FF070307h
	or	edx,0FF070307h
	mov	dword ptr [edi+4],ebx
	mov	dword ptr [edi],edx
	add	esi,4
	add	edi,8
	dec	ebp
	jg	L4F089A
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
decode_565toRGBA ENDP

decode_RGBAto565B PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebp,ecx
	shr	ebp,1
	jae	L4F0916
	mov	eax,dword ptr [esi]
	shr	eax,3
	shl	ax,1
	shl	al,2
	shl	ax,2
	shr	eax,5
	mov	word ptr [edi],ax
	add	esi,4
	add	edi,2
L4F0916:
	mov	eax,dword ptr [esi+4]
	mov	ecx,dword ptr [esi]
	shr	eax,3
	shr	ecx,3
	shl	ax,1
	shl	cx,1
	shl	al,2
	shl	cl,2
	shl	ax,2
	shl	cx,2
	shr	eax,5
	shr	ecx,5
	shl	eax,10h
	mov	ax,cx
	mov	dword ptr [edi],eax
	add	esi,8
	add	edi,4
	dec	ebp
	jg	L4F0916
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
decode_RGBAto565B ENDP

decode_565BtoRGBA PROC
	push	ebp
	mov	ebp,esp
	push	ds
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	ecx,dword ptr [ebp+10h]
	mov	ebp,ecx
	shr	ebp,1
	jae	L4F0982
	movzx	eax,word ptr [esi]
	shl	eax,5
	shr	ax,3
	shl	eax,1
	shr	al,2
	shl	eax,2
	mov	dword ptr [edi],eax
	add	esi,2
	add	edi,4
L4F0982:
	movzx	eax,word ptr [esi+2]
	movzx	ecx,word ptr [esi]
	shl	eax,5
	shl	ecx,5
	shr	ax,3
	shr	cx,3
	shl	eax,1
	shl	ecx,1
	shr	al,2
	shr	cl,2
	shl	eax,2
	shl	ecx,2
	mov	dword ptr [edi+4],eax
	mov	dword ptr [edi],ecx
	add	esi,4
	add	edi,8
	dec	ebp
	jg	L4F0982
	popad
	pop	es
	pop	ds
	pop	ebp
	ret
decode_565BtoRGBA ENDP

	END
