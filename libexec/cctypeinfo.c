/*
 *	Print architecture characteristics
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define t(type) \
	printf("%14s  %7zu  %7zu\n", #type, sizeof(type), __alignof__(type))

int main(void)
{
	printf("%14s  %7s  %7s\n", "TYPE", "SIZEOF", "ALIGNOF");
	t(char);
	t(short);
	t(int);
	t(long);
	t(long long);
	t(float);
	t(double);
	t(long double);
	t(void *);
	t(void (*)(void));
	t(wchar_t);
	t(uint8_t);
	t(uint16_t);
	t(uint32_t);
	t(uint64_t);
	return EXIT_SUCCESS;
}
