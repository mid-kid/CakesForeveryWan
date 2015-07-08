#include "svc.h"

__attribute__((naked))
void svcExitThread(void)
{
	asm
	(
		"svc 0x9\n\t"
		"bx lr"
	);
}

__attribute__((naked))
void svcSleepThread(uint32_t nanoSecsLo, uint32_t nanoSecsHi)
{
	asm
	(
		"svc 0x0A\n\t"
		"bx lr"
	);
}

__attribute__((naked))
void svcFlushProcessDataCache(Handle process, void const* addr, uint32_t size)
{
	asm
	(
		"svc 0x54\n\t"
		"bx lr"
	);
}

__attribute__((naked))
void svcBackdoor(void(*funcptr)(void))
{
	asm
	(
		"svc 0x7B\n\t"
		"bx lr"
	);
}

__attribute__((naked))
void svcCreateThread(Handle* thread, void(*entrypoint)(void), void* arg, void* stacktop, int32_t threadpriority, int32_t processorid)
{
	asm
	(
		"stmfd sp!, {r0, r4}\n\t"
		"ldr r0, [sp, #0x8]\n\t"
		"ldr r4, [sp, #0xC]\n\t"
		"svc 8\n\t"
		"ldr r2, [sp, #0x0]\n\t"
		"str r1, [r2]\n\t"
		"add sp, sp, #4\n\t"
		"ldr r4, [sp], #4\n\t"
		"bx lr"
	);
}

