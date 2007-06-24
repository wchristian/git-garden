/*
 *	prime.S - check if a number is prime
 *	x86 assembler demonstration program
 *	use with GNU AS
 *
 *	by Jan Engelhardt <jengelh [at] gmx de>, 2003 - 2006
 *	http://jengelh.hopto.org/
 *	released in the Public Domain
 */
.intel_syntax noprefix;
.section .rodata;
.L_LC0:
	.string "%u is a prime\n";
.L_LC1:
	.string "%u is not a prime (split by %u)\n";

.text;

.global main;
	.type   main, @function;
main:
	push    ebp;
	mov     ebp, esp;

	push    dword ptr [ebp+12];
	push    dword ptr [ebp+8];
	call    GetPrime;
	add     esp, 8;
	mov     edi, eax;

	mov     esi, eax;
	call    IsPrime;
	test    eax, eax;
	push    eax;
	jne     .L_main_noprime;

.L_main_isprime:
	push    edi;
	push    offset flat:.L_LC0;
	call    printf;
	add     esp, 8;
	jmp     .L_main_end;

.L_main_noprime:
	push    eax;
	push    edi;
	push    offset flat:.L_LC1;
	call    printf;
	add     esp, 12;

.L_main_end:
	pop     eax;
	leave;
	ret;

.globl GetPrime;
	.type   GetPrime, @function;
GetPrime:
	push    ebp;
	mov     ebp, esp;

	cmp     dword ptr [ebp+8], 2;
	jae     .L_GetArgs_run;
	mov     eax, 13;
	jmp     .L_GetArgs_end;

.L_GetArgs_run:
	mov     ebx, dword ptr [ebp+12];
	push    0;
	push    0;
	push    dword ptr [ebx+4];
	call    strtoul;
	add     esp, 12;

.L_GetArgs_end:
	leave;
	ret;

/*
 * In an attempt to improve IsPrime2 further, this function only runs up to
 * sqrt(N) rather than N/2. This is possible since if you split the number in
 * two integral factors, the maximum value of either (see them independently)
 * can be sqrt(N):
 * 
 * Imagine we take 100 as the number to check:
 *        100 = x * y      |T	    y strives to 0, while y >= x
 *    =>  100 = x ** 2     |sqrt()
 *   <=>    x = 10
 * 
 * Checking a prime near 2^31 thus only takes about 3 secs instead on a modest
 * CPU. (15+ secs for IsPrime2 on *that* modest CPU.)
 */

.globl IsPrime;
	.type   IsPrime, @function
IsPrime:
	push    ebp;
	mov     ebp, esp;
	sub     esp, 8;

	/* Early case exclusion */
	cmp     esi, 3;
	jbe     .L_IsPrime_IsPrime;

	mov     ecx, 2;
	test    si, 1;
	je      .L_IsPrime_NoPrime;

	cmp     esi, 7;
	jbe     .L_IsPrime_IsPrime;

	/* Set up FPU to round up */
	fstcw   word ptr [ebp-8];
	mov     ax, word ptr [ebp-8];
	and     ax, 0xF0FF;
	or      ax, 0x0B00;
	mov     word ptr [ebp-8], ax;
	fldcw   /* word ptr */ [ebp-8];

	/* Maximum count is ceil(sqrt(esi)) */
	mov     dword ptr [ebp-4], esi;
	fild    dword ptr [ebp-4];
	fsqrt;
	fistp   dword ptr [ebp-4];
	mov     ebx, dword ptr [ebp-4];

	/* Iterative loop */
	cmp     ebx, 3;
	jb      .L_IsPrime_IsPrime;
	mov     ecx, 3;

.L_IsPrime_loop:
	mov     eax, esi;
	xor     edx, edx;
	div     ecx;
	test    edx, edx;
	je      .L_IsPrime_NoPrime;
	add     ecx, 2;

	cmp     ecx, ebx;
	jbe     .L_IsPrime_loop;

.L_IsPrime_IsPrime:
	add     esp, 8;
	mov     eax, 0;
	leave;
	ret;

.L_IsPrime_NoPrime:
	add     esp, 8;
	mov     eax, ecx;
	leave;
	ret;
