﻿#include "Debug.h"
#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"
#include "StringView.h"
#include "Span.h"
#include "FixedArray.h"

INTRA_DISABLE_REDUNDANT_WARNINGS
#include <cstdlib>

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4668)
#endif

#include <windows.h>

#ifndef INTRA_DBGHELP

#else

#ifdef _MSC_VER
#pragma warning(disable: 4091)
#pragma comment(lib, "DbgHelp.lib")
#endif
#include <DbgHelp.h>

#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#else
#include <unistd.h>
#endif

namespace Intra {

void PrintDebugMessage(StringView message)
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	FixedArray<wchar_t> wmessage(message.Length()+1);
	const size_t wmessageLength = size_t(MultiByteToWideChar(CP_UTF8, 0, message.Data(),
		int(message.Length()), wmessage.Data(), int(message.Length())));
	wmessage[wmessageLength] = L'\0';
	OutputDebugStringW(wmessage.Data());
#else
	write(STDERR_FILENO, message.Data(), message.Length());
#endif
}

static void AppendUInt(Span<char>& dst, unsigned val)
{
	char lineStr[10];
	char* lineStrPtr = lineStr+10;
	do *--lineStrPtr = char('0' + val % 10), val /= 10;
	while(val != 0);
	while(lineStrPtr != lineStr && !dst.Empty()) dst.Put(*lineStrPtr++);
}

void PrintDebugMessage(StringView message, StringView file, unsigned line)
{
	FixedArray<char> buffer(14+file.Length()+message.Length());
	Span<char> dst = buffer;
	file.CopyToAdvance(dst);
	dst << '(';
	AppendUInt(dst, line);
	StringView("): ").CopyToAdvance(dst);
	message.CopyToAdvance(dst);
	PrintDebugMessage({buffer.Data(), dst.Data()});
}

bool IsDebuggerAttached()
{
#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
	return IsDebuggerPresent()!=FALSE;
#else
	return false;
#endif
}

#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Windows || defined(INTRA_DBGHELP))

static bool IsSeparatorChar(char c)
{
	const char* sepChars = ",.: \t\r\n";
	while(*sepChars) if(c == *sepChars++) return true;
	return false;
}

static bool ContainsMainFunction(StringView sym)
{
	StringView found = sym.Find("main");
	if(found.Empty()) return false;
	if(found.Data()>sym.Data() && !IsSeparatorChar(found.Data()[-1])) return false;
	if(found.Length()>4 && !IsSeparatorChar(found[4])) return false;
	return true;
}

#endif

}

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)

namespace Intra {

StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
#ifndef INTRA_DBGHELP
	(void)framesToSkip;
	(void)maxFrames;
	(void)untilMain;
	return null;
#else
	SymSetOptions(SYMOPT_DEFERRED_LOADS|SYMOPT_INCLUDE_32BIT_MODULES|SYMOPT_UNDNAME);
	if(!SymInitialize(GetCurrentProcess(), "http://msdl.microsoft.com/download/symbols", true))
		return null;

	if(maxFrames>50) maxFrames = 50;
	void* addrs[50] = {0};
	ushort frames = CaptureStackBackTrace(DWORD(2+framesToSkip), maxFrames, addrs, null);

	const auto dstStart = dst;
	for(ushort i=0; i<frames; i++)
	{
		ulong64 buffer[(sizeof(SYMBOL_INFO) + 1024 + sizeof(ulong64) - 1) / sizeof(ulong64)] = {0};
		SYMBOL_INFO* info = reinterpret_cast<SYMBOL_INFO*>(buffer);
		info->SizeOfStruct = sizeof(SYMBOL_INFO);
		info->MaxNameLen = 1024;

		DWORD64 displacement = 0;
		if(SymFromAddr(GetCurrentProcess(), size_t(addrs[i]), &displacement, info))
		{
			auto sym = StringView(info->Name, info->NameLen);
			sym.CopyToAdvance(dst);
			dst << '\n';
			if(untilMain && ContainsMainFunction(sym)) break;
		}
	}

	SymCleanup(GetCurrentProcess());

	return {dstStart.Data(), dst.Data()};
#endif
}

}

#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux)
#include <execinfo.h>

namespace Intra {

StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
	if(framesToSkip>50) return null;
	if(maxFrames+framesToSkip>50) maxFrames = 50-framesToSkip;
	void* pointerArr[50];
	size_t size = size_t(backtrace(pointerArr, int(maxFrames+framesToSkip)));
	char** strings = backtrace_symbols(pointerArr, int(size+framesToSkip));

	const auto dstStart = dst;
	for(size_t i=framesToSkip; i<size+framesToSkip; i++)
	{
		auto sym = StringView(strings[i]);
		sym.CopyToAdvance(dst);
		dst << '\n';
		if(untilMain && ContainsMainFunction(sym)) break;
	}

	free(strings);

	return {dstStart.Data(), dst.Data()};
}

}
#else

namespace Intra {

StringView GetStackTrace(Span<char>& dst, size_t framesToSkip, size_t maxFrames, bool untilMain)
{
	(void)framesToSkip; (void)maxFrames; (void)untilMain;
	return dst.TakeNone();
}

}

#endif

namespace Intra {

StringView BuildErrorMessage(Span<char>& dst, StringView func, StringView file, unsigned line, StringView info, size_t stackFramesToSkip)
{
	const StringView msg = dst;
	dst << file << '(';
	AppendUInt(dst, line);
	dst << "): internal error detected in function\n" << func << '\n' << info << "\n\nStack trace:\n";
	const StringView stackTrace = GetStackTrace(dst, 1+stackFramesToSkip, 50);
	if(stackTrace.Empty())
		dst << "<Not supported on this platform>\n";
	return {msg.Data(), dst.Data()};
}

void FatalErrorMessageAbort(SourceInfo srcInfo, StringView msg)
{
#if(INTRA_MINEXE>=3)
	(void)srcInfo, (void)msg;
	abort();
#else
	//Проверим, что это не рекурсивная ошибка, возникшая при выводе другой ошибки
	static bool was = false;
	if(was) return;
	was = true;

	char msgBuffer[16384];
	Span<char> msgBuf = SpanOfBuffer(msgBuffer);
	const StringView fullMsg = BuildErrorMessage(msgBuf, srcInfo.Function, srcInfo.File, srcInfo.Line, msg, 2);

	PrintDebugMessage(fullMsg);

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows)
	FixedArray<wchar_t> wbuffer(msg.Length() + 1);
	LPWSTR wmessage = wbuffer.Data();
	int wmessageLength = MultiByteToWideChar(CP_UTF8, 0, fullMsg.Data(), int(fullMsg.Length()), wmessage, int(fullMsg.Length()));
	wmessage[wmessageLength] = L'\0';
	MessageBoxW(null, wmessage, L"Critical error", MB_ICONERROR);
#endif

	exit(1);
#endif
}

FatalErrorCallbackType gFatalErrorCallback = FatalErrorMessageAbort;

void CallFatalErrorCallback(SourceInfo srcInfo, StringView msg)
{gFatalErrorCallback(srcInfo, msg);}

void CallFatalErrorCallback(SourceInfo srcInfo, const char* msg)
{gFatalErrorCallback(srcInfo, msg);}

}

