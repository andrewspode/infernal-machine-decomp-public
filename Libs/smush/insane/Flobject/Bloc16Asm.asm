; Bloc16Asm.asm: the blocky16 frame-object codec's block decoder (retail 0x004EDEF0-0x004EE6A0, data
; 0x0054DC70-0x0054DCA8), assembled with MASM 6.13.
;
; dbloc16_Codec2 decodes codec 2's 8x8 blocks: each opcode byte selects a motion-vector copy from the previous
; or delta frame, a solid fill, a two-colour glyph or a split into four 4x4 (and then 2x2) blocks.
; D_bloc16.c calls it (0x004E9271). dbloc16_Interpolate (a 2x upscale through a pair-interpolation table)
; has no caller in the image. The block procedures are internal subroutines with a register interface
; (esi = opcode stream, edi = destination, ebp = line length).

	.386
	.model flat, c

	.data
dbloc16_reserved	dw 7 dup (0)		; nothing in the image references these words
dbloc16_prevOffset	dd 0			; previous frame - current frame, in pixels * 2
dbloc16_deltaOffset	dd 0			; delta frame - current frame, in pixels * 2
dbloc16_width		dd 0
dbloc16_motion		dd 0			; motion-vector table (words)
dbloc16_colors		dd 0			; fill colours for opcodes 0F9h-0FCh
dbloc16_glyph4		dd 0			; 4x4 glyph table
dbloc16_glyph8		dd 0			; 8x8 glyph table
dbloc16_palette		dd 0			; 8-bit to 16-bit palette
dbloc16_rowSkip		dd 0			; bytes from the end of a block row to the next
dbloc16_blocksX		dw 0
dbloc16_countX		dw 0
dbloc16_blocksY		dw 0

	.code

dbloc16_Interpolate PROC
	push	ebp
	mov	ebp,esp
	push	es
	pushad
	push	ds
	pop	es
	mov	edi,dword ptr [ebp+8]
	mov	esi,dword ptr [ebp+0Ch]
	mov	edx,dword ptr [ebp+10h]
	mov	eax,dword ptr [ebp+14h]
	mov	ebx,dword ptr [ebp+18h]
	mov	ebp,eax
	push	edi
	push	ebp
	add	edi,edx
L4EDF0C:
	mov	al,byte ptr [esi]
	mov	ah,al
	inc	esi
	mov	word ptr [edi],ax
	add	edi,2
	push	edx
	sub	edx,2
	ALIGN 4
L4EDF1C:
	mov	ah,al
	mov	al,byte ptr [esi]
	mov	cl,byte ptr [ebx+eax]
	inc	esi
	mov	ch,al
	mov	word ptr [edi],cx
	add	edi,2
	sub	edx,2
	jg	L4EDF1C
	pop	edx
	add	edi,edx
	sub	ebp,2
	jg	L4EDF0C
	pop	ebp
	pop	edi
	mov	esi,edi
	add	esi,edx
	mov	ecx,edx
	rep movsb
	add	edi,edx
	mov	ecx,edx
	neg	ecx
	xor	eax,eax
	sub	ebp,2
L4EDF4E:
	push	ebp
	mov	ebp,edx
	nop
L4EDF52:
	mov	al,byte ptr [edi+ecx]
	mov	ah,byte ptr [edi+edx]
	mov	al,byte ptr [ebx+eax]
	mov	byte ptr [edi],al
	inc	edi
	sub	ebp,1
	jg	L4EDF52
	pop	ebp
	add	edi,edx
	sub	ebp,2
	jg	L4EDF4E
	popad
	pop	es
	pop	ebp
	ret
	ALIGN 4
dbloc16_Interpolate ENDP

dbloc16_Codec2 PROC
	push	ebp
	mov	ebp,esp
	push	es
	pushad
	push	ds
	pop	es
	mov	eax,dword ptr [ebp+10h]
	add	eax,eax
	mov	dword ptr dbloc16_prevOffset,eax
	mov	eax,dword ptr [ebp+14h]
	add	eax,eax
	mov	dword ptr dbloc16_deltaOffset,eax
	mov	eax,dword ptr [ebp+18h]
	mov	dword ptr dbloc16_width,eax
	mov	eax,dword ptr [ebp+1Ch]
	mov	dword ptr dbloc16_motion,eax
	mov	eax,dword ptr [ebp+20h]
	sub	eax,0F9h
	sub	eax,0F9h
	mov	dword ptr dbloc16_colors,eax
	mov	eax,dword ptr [ebp+2Ch]
	mov	dword ptr dbloc16_glyph4,eax
	mov	eax,dword ptr [ebp+30h]
	mov	dword ptr dbloc16_glyph8,eax
	mov	eax,dword ptr [ebp+34h]
	mov	dword ptr dbloc16_palette,eax
	mov	esi,dword ptr [ebp+0Ch]
	mov	edi,dword ptr [ebp+8]
	mov	eax,dword ptr [ebp+24h]
	add	eax,7
	shr	eax,3
	mov	byte ptr dbloc16_blocksY,al
	mov	eax,dword ptr [ebp+18h]
	add	eax,7
	shr	eax,3
	mov	byte ptr dbloc16_blocksX,al
	mov	ebp,dword ptr [ebp+18h]
	add	ebp,ebp
	mov	dword ptr dbloc16_rowSkip,ebp
	shl	dword ptr dbloc16_rowSkip,3
	sub	dword ptr dbloc16_rowSkip,ebp
L4EDFFF:
	mov	al,byte ptr dbloc16_blocksX
	mov	byte ptr dbloc16_countX,al
L4EE009:
	call	dbloc16_Block8
	add	edi,10h
	dec	byte ptr dbloc16_countX
	jne	L4EE009
	add	edi,dword ptr dbloc16_rowSkip
	dec	byte ptr dbloc16_blocksY
	jne	L4EDFFF
	popad
	pop	es
	pop	ebp
	ret
	ALIGN 4
dbloc16_Codec2 ENDP

dbloc16_Block8 PROC PRIVATE
	mov	al,byte ptr [esi]
	cmp	al,0F5h
	ja	L4EE148
	jne	L4EE043
	movsx	ecx,word ptr [esi+1]
	add	ecx,ecx
	add	esi,3
	jmp	L4EE057
L4EE043:
	and	eax,0FFh
	mov	ecx,dword ptr dbloc16_motion
	add	esi,1
	movsx	ecx,word ptr [ecx+eax*2]
	add	ecx,ecx
L4EE057:
	push	edi
	push	esi
	add	ecx,dword ptr dbloc16_prevOffset
	lea	esi,[edi+ebp*4]
	mov	edx,ecx
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	eax,dword ptr [esi+ecx+8]
	mov	ebx,dword ptr [edi+edx+8]
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	eax,dword ptr [esi+ecx+0Ch]
	mov	ebx,dword ptr [edi+edx+0Ch]
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	eax,dword ptr [esi+ecx+8]
	mov	ebx,dword ptr [edi+edx+8]
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	eax,dword ptr [esi+ecx+0Ch]
	mov	ebx,dword ptr [edi+edx+0Ch]
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	eax,dword ptr [esi+ecx+8]
	mov	ebx,dword ptr [edi+edx+8]
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	eax,dword ptr [esi+ecx+0Ch]
	mov	ebx,dword ptr [edi+edx+0Ch]
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	eax,dword ptr [esi+ecx+8]
	mov	ebx,dword ptr [edi+edx+8]
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	eax,dword ptr [esi+ecx+0Ch]
	mov	ebx,dword ptr [edi+edx+0Ch]
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	pop	esi
	pop	edi
	ret
	ALIGN 4
L4EE148:
	cmp	al,0FFh
	jne	L4EE170
	add	esi,1
	push	edi
	call	dbloc16_Block4
	add	edi,8
	call	dbloc16_Block4
	lea	edi,[edi+ebp*4-8]
	call	dbloc16_Block4
	add	edi,8
	call	dbloc16_Block4
	pop	edi
	ret
L4EE170:
	cmp	al,0FDh
	ja	L4EE1B4
	je	L4EE199
	cmp	al,0F9h
	jb	L4EE23C
	and	eax,0FFh
	mov	ecx,dword ptr dbloc16_colors
	add	esi,1
	mov	eax,dword ptr [ecx+eax*2]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	jmp	L4EE1C4
L4EE199:
	xor	eax,eax
	mov	al,byte ptr [esi+1]
	mov	ecx,dword ptr dbloc16_palette
	add	esi,2
	mov	eax,dword ptr [ecx+eax*2]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	jmp	L4EE1C4
L4EE1B4:
	xor	eax,eax
	mov	eax,dword ptr [esi+1]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	add	esi,3
L4EE1C4:
	push	edi
	push	esi
	mov	eax,ebx
	lea	esi,[edi+ebp*4]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	sub	esi,ebp
	sub	edi,ebp
	sub	esi,ebp
	sub	edi,ebp
	pop	esi
	pop	edi
	ret
	ALIGN 4
L4EE23C:
	cmp	al,0F8h
	je	L4EE26C
	cmp	al,0F7h
	jne	L4EE2CC
	xor	edx,edx
	mov	bx,word ptr [esi+2]
	mov	eax,dword ptr dbloc16_palette
	mov	dl,bh
	mov	cx,word ptr [eax+edx*2]
	shl	ecx,10h
	mov	dl,bl
	mov	cx,word ptr [eax+edx*2]
	xor	ebx,ebx
	mov	bl,byte ptr [esi+1]
	add	esi,4
	jmp	L4EE277
L4EE26C:
	mov	ecx,dword ptr [esi+2]
	xor	ebx,ebx
	mov	bl,byte ptr [esi+1]
	add	esi,6
L4EE277:
	lea	eax,[ebx*4]
	shl	ebx,7
	add	eax,ebx
	add	eax,ebx
	add	eax,ebx
	add	eax,dword ptr dbloc16_glyph8
	push	ebp
	xor	ebx,ebx
	mov	bl,byte ptr [eax+180h]
	mov	ebp,ebx
	mov	edx,eax
L4EE29A:
	movzx	ebx,word ptr [edx]
	mov	word ptr [edi+ebx*2],cx
	add	edx,2
	dec	ebp
	jg	L4EE29A
	xor	ebx,ebx
	mov	bl,byte ptr [eax+181h]
	mov	ebp,ebx
	shr	ecx,10h
	lea	edx,[eax+80h]
L4EE2BA:
	movzx	ebx,word ptr [edx]
	mov	word ptr [edi+ebx*2],cx
	add	edx,2
	dec	ebp
	jg	L4EE2BA
	pop	ebp
	ret
	ALIGN 4
L4EE2CC:
	add	esi,1
	push	edi
	push	esi
	mov	ecx,dword ptr dbloc16_deltaOffset
	lea	esi,[edi+ebp*4]
	mov	edx,ecx
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	eax,dword ptr [esi+ecx+8]
	mov	ebx,dword ptr [edi+edx+8]
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	eax,dword ptr [esi+ecx+0Ch]
	mov	ebx,dword ptr [edi+edx+0Ch]
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	eax,dword ptr [esi+ecx+8]
	mov	ebx,dword ptr [edi+edx+8]
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	eax,dword ptr [esi+ecx+0Ch]
	mov	ebx,dword ptr [edi+edx+0Ch]
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	eax,dword ptr [esi+ecx+8]
	mov	ebx,dword ptr [edi+edx+8]
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	eax,dword ptr [esi+ecx+0Ch]
	mov	ebx,dword ptr [edi+edx+0Ch]
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	mov	eax,dword ptr [esi+ecx+8]
	mov	ebx,dword ptr [edi+edx+8]
	mov	dword ptr [esi+8],eax
	mov	dword ptr [edi+8],ebx
	mov	eax,dword ptr [esi+ecx+0Ch]
	mov	ebx,dword ptr [edi+edx+0Ch]
	mov	dword ptr [esi+0Ch],eax
	mov	dword ptr [edi+0Ch],ebx
	add	esi,ebp
	add	edi,ebp
	pop	esi
	pop	edi
	ret
	ALIGN 4
	ret
	ALIGN 4
dbloc16_Block8 ENDP

dbloc16_Block4 PROC PRIVATE
	mov	al,byte ptr [esi]
	cmp	al,0F5h
	ja	L4EE430
	jne	L4EE3D7
	movsx	ecx,word ptr [esi+1]
	add	ecx,ecx
	add	esi,3
	jmp	L4EE3EB
L4EE3D7:
	and	eax,0FFh
	mov	ecx,dword ptr dbloc16_motion
	add	esi,1
	movsx	ecx,word ptr [ecx+eax*2]
	add	ecx,ecx
L4EE3EB:
	push	edi
	push	esi
	add	ecx,dword ptr dbloc16_prevOffset
	lea	esi,[edi+ebp*2]
	mov	edx,ecx
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	add	esi,ebp
	add	edi,ebp
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	pop	esi
	pop	edi
	ret
	ALIGN 4
L4EE430:
	cmp	al,0FFh
	jne	L4EE458
	add	esi,1
	push	edi
	call	dbloc16_Block2
	add	edi,4
	call	dbloc16_Block2
	lea	edi,[edi+ebp*2-4]
	call	dbloc16_Block2
	add	edi,4
	call	dbloc16_Block2
	pop	edi
	ret
L4EE458:
	cmp	al,0FDh
	ja	L4EE498
	je	L4EE47D
	cmp	al,0F9h
	jb	L4EE4CC
	and	eax,0FFh
	mov	ecx,dword ptr dbloc16_colors
	add	esi,1
	mov	eax,dword ptr [ecx+eax*2]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	jmp	L4EE4A6
L4EE47D:
	xor	eax,eax
	mov	al,byte ptr [esi+1]
	mov	ecx,dword ptr dbloc16_palette
	add	esi,2
	mov	eax,dword ptr [ecx+eax*2]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	jmp	L4EE4A6
L4EE498:
	mov	eax,dword ptr [esi+1]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	add	esi,3
L4EE4A6:
	mov	eax,ebx
	mov	edx,edi
	mov	ecx,ebp
	mov	dword ptr [edi],ebx
	mov	dword ptr [edx+4],eax
	add	edi,ebp
	add	edx,ecx
	mov	dword ptr [edi],ebx
	mov	dword ptr [edx+4],eax
	mov	dword ptr [edi+ebp],ebx
	mov	dword ptr [edx+ecx+4],eax
	mov	dword ptr [edi+ebp*2],ebx
	mov	dword ptr [edx+ecx*2+4],eax
	sub	edi,ebp
	ret
L4EE4CC:
	cmp	al,0F8h
	je	L4EE4F8
	cmp	al,0F7h
	jne	L4EE540
	xor	edx,edx
	mov	bx,word ptr [esi+2]
	mov	eax,dword ptr dbloc16_palette
	mov	dl,bh
	mov	cx,word ptr [eax+edx*2]
	shl	ecx,10h
	mov	dl,bl
	mov	cx,word ptr [eax+edx*2]
	xor	eax,eax
	mov	al,byte ptr [esi+1]
	add	esi,4
	jmp	L4EE503
L4EE4F8:
	mov	ecx,dword ptr [esi+2]
	xor	eax,eax
	mov	al,byte ptr [esi+1]
	add	esi,6
L4EE503:
	shl	eax,7
	add	eax,dword ptr dbloc16_glyph4
	push	ebp
	xor	ebx,ebx
	mov	bl,byte ptr [eax+60h]
	mov	ebp,ebx
	mov	edx,eax
L4EE516:
	movzx	ebx,word ptr [edx]
	mov	word ptr [edi+ebx*2],cx
	add	edx,2
	dec	ebp
	jg	L4EE516
	xor	ebx,ebx
	mov	bl,byte ptr [eax+61h]
	mov	ebp,ebx
	shr	ecx,10h
	lea	edx,[eax+20h]
L4EE530:
	movzx	ebx,word ptr [edx]
	mov	word ptr [edi+ebx*2],cx
	add	edx,2
	dec	ebp
	jg	L4EE530
	pop	ebp
	ret
	ALIGN 4
L4EE540:
	add	esi,1
	push	edi
	push	esi
	mov	ecx,dword ptr dbloc16_deltaOffset
	lea	esi,[edi+ebp*2]
	mov	edx,ecx
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	add	esi,ebp
	add	edi,ebp
	mov	eax,dword ptr [esi+ecx]
	mov	ebx,dword ptr [edi+edx]
	mov	dword ptr [esi],eax
	mov	dword ptr [edi],ebx
	mov	eax,dword ptr [esi+ecx+4]
	mov	ebx,dword ptr [edi+edx+4]
	mov	dword ptr [esi+4],eax
	mov	dword ptr [edi+4],ebx
	pop	esi
	pop	edi
	ret
	ALIGN 4
	ret
	ALIGN 4
dbloc16_Block4 ENDP

dbloc16_Block2 PROC PRIVATE
	mov	al,byte ptr [esi]
	cmp	al,0F5h
	ja	L4EE5C8
	jne	L4EE59F
	movsx	ecx,word ptr [esi+1]
	add	ecx,ecx
	add	esi,3
	jmp	L4EE5B3
L4EE59F:
	and	eax,0FFh
	mov	ecx,dword ptr dbloc16_motion
	add	esi,1
	movsx	ecx,word ptr [ecx+eax*2]
	add	ecx,ecx
L4EE5B3:
	add	ecx,dword ptr dbloc16_prevOffset
	mov	eax,dword ptr [edi+ecx]
	mov	dword ptr [edi],eax
	add	edi,ebp
	mov	eax,dword ptr [edi+ecx]
	mov	dword ptr [edi],eax
	sub	edi,ebp
	ret
L4EE5C8:
	cmp	al,0FFh
	je	L4EE5D0
	cmp	al,0F8h
	jne	L4EE5E0
L4EE5D0:
	mov	eax,dword ptr [esi+1]
	mov	dword ptr [edi],eax
	mov	ebx,dword ptr [esi+5]
	mov	dword ptr [edi+ebp],ebx
	add	esi,9
	ret
L4EE5E0:
	cmp	al,0FDh
	ja	L4EE62C
	je	L4EE60C
	cmp	al,0F9h
	jb	L4EE644
	and	eax,0FFh
	mov	ecx,dword ptr dbloc16_colors
	add	esi,1
	mov	eax,dword ptr [ecx+eax*2]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	mov	dword ptr [edi],ebx
	mov	dword ptr [edi+ebp],ebx
	ret
	ALIGN 4
L4EE60C:
	xor	eax,eax
	mov	al,byte ptr [esi+1]
	mov	ecx,dword ptr dbloc16_palette
	add	esi,2
	mov	eax,dword ptr [ecx+eax*2]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	mov	dword ptr [edi],ebx
	mov	dword ptr [edi+ebp],ebx
	ret
L4EE62C:
	mov	eax,dword ptr [esi+1]
	mov	ebx,eax
	shl	ebx,10h
	mov	bx,ax
	add	esi,3
	mov	dword ptr [edi],ebx
	mov	dword ptr [edi+ebp],ebx
	ret
	ALIGN 4
L4EE644:
	cmp	al,0F7h
	jne	L4EE680
	mov	ecx,dword ptr dbloc16_palette
	mov	ebx,dword ptr [esi+1]
	xor	eax,eax
	mov	al,bl
	mov	edx,dword ptr [ecx+eax*2]
	mov	word ptr [edi],dx
	mov	al,bh
	mov	edx,dword ptr [ecx+eax*2]
	mov	word ptr [edi+2],dx
	shr	ebx,10h
	mov	al,bl
	mov	edx,dword ptr [ecx+eax*2]
	mov	word ptr [edi+ebp],dx
	mov	al,bh
	mov	edx,dword ptr [ecx+eax*2]
	add	esi,5
	mov	word ptr [edi+ebp+2],dx
	ret
	ALIGN 4
L4EE680:
	cmp	al,0F6h
	jb	L4EE69C
	add	esi,1
	mov	ecx,dword ptr dbloc16_deltaOffset
	mov	eax,dword ptr [edi+ecx]
	mov	dword ptr [edi],eax
	add	edi,ebp
	mov	eax,dword ptr [edi+ecx]
	mov	dword ptr [edi],eax
	sub	edi,ebp
	ret
L4EE69C:
	ret
	ALIGN 4
dbloc16_Block2 ENDP

	END
