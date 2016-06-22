
bits	64
cpu		ia64

global	test_asm

;-----------------------------------------------------------------
; Name:			test_asm
; Purpose:		Get CPUID information
; Params:		rdi = ptr to memory for CPUID info
;				rsi = eax
;				rdx = ecx
;-----------------------------------------------------------------

test_asm:
	push r10

	mov r10, rdi

	mov rax, rsi
	mov rcx, rdx

	cpuid

	mov [r10],    rax
	mov [r10+8],  rbx
	mov [r10+16], rcx
	mov [r10+24], rdx

	pop r10

	ret

