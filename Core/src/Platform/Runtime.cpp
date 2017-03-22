﻿#include "Platform/PlatformInfo.h"
#include "Platform/Compatibility.h"
#include "Container/Sequential/Array.h"
#include "Container/Sequential/String.h"
#include "Platform/CppWarnings.h"
#include "Platform/Runtime.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)

struct android_app;

namespace Intra {
	android_app *g_GlobalAndroidApp = null;
}

#ifndef INTRA_NO_CMAIN

extern "C" int main(int argc, const char* argv[]);
void android_main(struct android_app* state)
{
	Intra::g_GlobalAndroidApp = state;
	const char* argv[] = {"program"};
	main(1, &argv[0]);
}

#endif

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#include <Windows.h>
#include "Platform/Environment.h"

#ifndef INTRA_NO_WINDOWS_MAIN
extern "C" int INTRA_CRTDECL main(int argc, const char* argv[]);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	using namespace Intra;
	auto args = GetCommandLineArguments();
	String argStr = ToString(args, "\0", "", "");
	Array<const char*> argv(args.Length());
	size_t start=0;
	for(auto&& arg: args)
	{
		argv.AddLast(argStr.Data()+start);
		start += 1+arg.Length();
	}
	if(argv.Empty()) argv.AddLast("");
	return main(int(args.Length()), &argv[0]);
}
#endif

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)

extern "C" int __cxa_thread_atexit(void(*func)(), void* obj, void* dsoSymbol)
{
	(void)func; (void)obj; (void)dsoSymbol;
	return 0;
}

#endif


#if(defined(_MSC_VER) && defined(INTRA_MINIMIZE_CRT))

#define _CRT_RAND_S
#include <stdlib.h>
#include "Platform/Debug.h"
namespace std {

unsigned int INTRA_CRTDECL _Random_device()
{
	unsigned int ans;
	if(rand_s(&ans)) INTRA_INTERNAL_ERROR("invalid random_device value");
	return (ans);
}

}

#if _MSC_VER>=1800

#include <stdlib.h>

//extern "C" int _imp___fdtest(float*) {return -1;} //Это заглушка для недостающего символа. Она может вызвать ошибки с математической библиотекой

namespace std
{
	void _cdecl _Xbad_alloc() { abort(); }
	void _cdecl _Xbad_function_call() { abort(); }
	void _cdecl _Xout_of_range(const char*) { abort(); }
	void _cdecl _Xlength_error(const char*) { abort(); }
	const char* _cdecl _Syserror_map(int) { return "error"; }
	const char* _cdecl _Winerror_map(int) { return "error"; }
}
extern "C" void _cdecl _except_handler4_common() {}

#if !defined(INTRA_INLINE_MATH) && defined(INTRA_MINIMIZE_CRT)
#include <math.h>
extern "C"
{
#ifndef _ACRTIMP
#define _ACRTIMP
#endif
	_Check_return_ _ACRTIMP double INTRA_CRTDECL round(_In_ double x);
	_Check_return_ _ACRTIMP float INTRA_CRTDECL roundf(_In_ float x);
	_Check_return_ _ACRTIMP double INTRA_CRTDECL round(_In_ double x) {return ::floor(x+0.5);}
	_Check_return_ _ACRTIMP float INTRA_CRTDECL roundf(_In_ float x) {return ::floorf(x+0.5f);}
}
#endif

#endif

#if(_MSC_VER>=1900)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

//thread_safe_statics.obj;utility_desktop.obj

void __CRTDECL operator delete(void* block, decltype(sizeof(0))) noexcept
{
	operator delete(block);
}

extern "C" {

FILE* _cdecl __acrt_iob_func(unsigned int id)
{
	static FILE* const stds[]={_fdopen(0, "r"), _fdopen(1, "w"), _fdopen(2, "w")};
	return stds[id];
}

BOOL INTRA_CRTDECL __vcrt_InitializeCriticalSectionEx(LPCRITICAL_SECTION critical_section, DWORD spin_count, DWORD flags)
{
	/*if(auto const initialize_critical_section_ex = try_get_InitializeCriticalSectionEx())
	{
		return initialize_critical_section_ex(critical_section, spin_count, flags);
	}*/
	(void)flags;
	return InitializeCriticalSectionAndSpinCount(critical_section, spin_count);
}

void INTRA_CRTDECL terminate() {abort();}
void INTRA_CRTDECL _invalid_parameter_noinfo_noreturn() {abort();}
void _fastcall _guard_check_icall(unsigned int) {}

}

#endif

#ifdef INTRA_QIFIST
//Выполнить это при запуске, если используется /QIfist
inline int SetFloatingPointRoundingToTruncate()
{
	short control_word, control_word2;
	__asm
	{
		fstcw   control_word                // store fpu control word
		mov     dx, word ptr[control_word]
		or      dx, 0x0C00                  // rounding: truncate
		mov     control_word2, dx
		fldcw   control_word2               // load modfied control word
	}
	return 0;
}

static int GLOBAL = SetFloatingPointRoundingToTruncate();
#endif

#endif

#ifdef INTRA_NO_CRT

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifdef _MSC_VER
#define INTRA_NOT_LINK_CRT_LIB

extern "C" const int _fltused = 0;

__declspec(naked) void _ftol2()
{
	__asm
	{
		fistp qword ptr[esp-8]
		mov   edx, [esp-4]
		mov   eax, [esp-8]
		ret
	}
}

__declspec(naked) void _ftol2_sse()
{
	__asm
	{
		fistp dword ptr[esp-4]
		mov   eax, [esp-4]
		ret
	}
}


extern "C" int INTRA_CRTDECL _purecall() {return 0;}


extern "C" {

#pragma function(memset)
void* memset(void* dst, int c, size_t count)
{
	char *bytes = (char*)dst;
	while(count--) *bytes++ = (char)c;
	return dst;
}

#pragma function(memcpy)
void* memcpy(void* dst, const void* src, size_t count)
{
	char *dst8 = (char*)dst;
	const char *src8 = (const char*)src;
	while(count--) *dst8++ = *src8++;
	return dst;
}

}


#define _CRTALLOC(x) __declspec(allocate(x))
typedef void(INTRA_CRTDECL *_PVFV)();

#pragma data_seg(".CRT$XIA")    /* C initializers */
_PVFV __xi_a[] ={NULL};
#pragma data_seg(".CRT$XIZ")
_PVFV __xi_z[] ={NULL};
#pragma data_seg(".CRT$XCA")    /* C++ initializers */
_PVFV __xc_a[] ={NULL};
#pragma data_seg(".CRT$XCZ")
_PVFV __xc_z[] ={NULL};
#pragma data_seg(".CRT$XPA")    /* C pre-terminators */
_PVFV __xp_a[] ={NULL};
#pragma data_seg(".CRT$XPZ")
_PVFV __xp_z[] ={NULL};
#pragma data_seg(".CRT$XTA")    /* C terminators */
_PVFV __xt_a[] ={NULL};
#pragma data_seg(".CRT$XTZ")
_PVFV __xt_z[] ={NULL};
#pragma data_seg()

/*extern _CRTALLOC(".CRT$XIA") _PVFV __xi_a[];
extern _CRTALLOC(".CRT$XIZ") _PVFV __xi_z[];    // C initializers
extern _CRTALLOC(".CRT$XCA") _PVFV __xc_a[];
extern _CRTALLOC(".CRT$XCZ") _PVFV __xc_z[];    // C++ initializers

extern _CRTALLOC(".CRT$XPA") _PVFV __xp_a[];
extern _CRTALLOC(".CRT$XPZ") _PVFV __xp_z[];
extern _CRTALLOC(".CRT$XTA") _PVFV __xt_a[];
extern _CRTALLOC(".CRT$XTZ") _PVFV __xt_z[];*/


#ifndef CRTDLL
static
#endif
void INTRA_CRTDECL _initterm(_PVFV* pfbegin, _PVFV* pfend)
{
	while(pfbegin<pfend)
	{
		if(*pfbegin!=null) (**pfbegin)();
		++pfbegin;
	}
}

HANDLE g_hHeap;

extern "C" int INTRA_CRTDECL main(int argc, char* argv[]);
extern "C" void mainCRTStartup()
{
	g_hHeap = HeapCreate(0, 8*1048576, 3072*1048576);
	_initterm(__xi_a, __xi_z);
	_initterm(__xc_a, __xc_z);

	int argc=0;
	char* argv[]={""};
	main(argc, argv);

	/* and finally... */
	_initterm(__xp_a, __xp_z);    /* Pre-termination (C++?) */
	_initterm(__xt_a, __xt_z);    /* Termination */
	HeapDestroy(g_hHeap);
}

#endif

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
void* INTRA_CRTDECL malloc(size_t bytes)
{return HeapAlloc(g_hHeap, 0, bytes);}

void* INTRA_CRTDECL realloc(void* ptr, size_t bytes)
{return HeapReAlloc(g_hHeap, 0, ptr, bytes);}

void INTRA_CRTDECL free(void* ptr)
{HeapFree(g_hHeap, 0, ptr);}
#else

#endif

void* INTRA_CRTDECL operator new(size_t bytes) noexcept
{return malloc(bytes);}

void INTRA_CRTDECL operator delete(void* block) noexcept
{free(block);}

void INTRA_CRTDECL operator delete(void* block, size_t) noexcept
{operator delete(block);}

#endif

INTRA_WARNING_POP
