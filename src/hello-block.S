	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 12, 0	sdk_version 12, 3
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, ___block_literal_global@PAGE
	add	x8, x8, ___block_literal_global@PAGEOFF
	str	x8, [sp, #8]
	ldr	x0, [sp, #8]
	ldr	x8, [x0, #16]
	blr	x8
	mov	w0, #0
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	add	sp, sp, #32
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function __main_block_invoke
___main_block_invoke:                   ; @__main_block_invoke
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	x8, x0
	str	x8, [sp, #8]
	str	x0, [sp]
	adrp	x0, l_.str.1@PAGE
	add	x0, x0, l_.str.1@PAGEOFF
	bl	_printf
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	add	sp, sp, #32
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"v8@?0"

	.section	__DATA,__const
	.p2align	3                               ; @__block_descriptor_tmp
___block_descriptor_tmp:
	.quad	0                               ; 0x0
	.quad	32                              ; 0x20
	.quad	l_.str
	.quad	0

	.p2align	3                               ; @__block_literal_global
___block_literal_global:
	.quad	__NSConcreteGlobalBlock
	.long	1342177280                      ; 0x50000000
	.long	0                               ; 0x0
	.quad	___main_block_invoke
	.quad	___block_descriptor_tmp

	.section	__TEXT,__cstring,cstring_literals
l_.str.1:                               ; @.str.1
	.asciz	"Hello, Blocks!!\n"

.subsections_via_symbols
