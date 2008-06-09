/*
 *	sym_word
 *	Check if a word is symmetric (oh, you call that palindrome...)
 *	x86 assembler demonstration program
 *
 *	written by Jan Engelhardt <jengelh [at] medozas de>, 2003 - 2007
 *	http://jengelh.medozas.de/
 *	released in the Public Domain
 */

.intel_syntax noprefix;
.section .rodata;
.L_LC0:
	.string "\"%s\" is symmetric\n";
.L_LC1:
	.string "\"%s\" is not symmetric\n";

.text;

.global main;
	.type   main, @function;
main:
	push    ebp;
	mov     ebp, esp;
	jmp     .L_main_cond;

.L_main_procloop:
	push    ebx;
	call    isSymmetric;
	add     esp, 4;

	test    eax, eax;
	je      .L_main_nosym;

	push    ebx;
	push    offset flat:.L_LC0;
	call    printf;
	add     esp, 4;
	jmp     .L_main_cond;

.L_main_nosym:
	push    ebx;
	push    offset flat:.L_LC1;
	call    printf;
	add     esp, 4;

.L_main_cond:
	add     dword ptr [ebp+12], 4;
	mov     ebx, dword ptr [ebp+12];
	mov     ebx, dword ptr [ebx];
	cmp     ebx, 0;
	jne     .L_main_procloop;

	mov     eax, 0;
	leave;
	ret;

.globl isSymmetric;
	.type	isSymmetric, @function;
isSymmetric:
	push	ebp;
	mov     ebp, esp;

	/* ecx = strlen(cp1); */
	mov     esi, dword ptr [ebp+8];
	push    esi;
	call    strlen;
	add     esp, 4;
	mov     ecx, eax;

	/* cp2 = cp1 + strlen(ecx) - 1; */
	mov     edi, esi;
	add     edi, ecx;
	dec     edi;

	/* ecx >>= 1; */
	shr     ecx, 1;
	inc     ecx; /* loop = while (--ecx) rather than ecx-- */
	jmp     .L_isSymmetric_cond;

.L_isSymmetric_loop:
	/* if (*cp1++ != *cp2--) */
	mov     al, byte ptr [esi];
	mov     ah, byte ptr [edi];
	inc     esi;
	dec     edi;
	cmp     al, ah;
	jne     .L_isSymmetric_NO;

.L_isSymmetric_cond:
	/* while (ecx--) */
	loop    .L_isSymmetric_loop;
	mov     eax, 1;
	leave;
	ret;

.L_isSymmetric_NO:
	mov     eax, 0;
	leave;
	ret;
