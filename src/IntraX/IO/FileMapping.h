#pragma once

#include "Intra/Core.h"

#include "Intra/Range/Span.h"
#include "Intra/Range/StringView.h"
#include "IntraX/System/Error.h"

#include "IntraX/Container/Sequential/String.h"

namespace Intra { INTRA_BEGIN
class BasicFileMapping
{
public:
	BasicFileMapping(BasicFileMapping&& rhs):
		mData(rhs.mData)
#ifdef INTRA_DEBUG
		, mFilePath(Move(rhs.mFilePath))
#endif
	{rhs.mData = nullptr;}

	BasicFileMapping& operator=(BasicFileMapping&& rhs)
	{
		Swap(mData, rhs.mData);
#ifdef INTRA_DEBUG
		mFilePath = Move(rhs.mFilePath);
#endif
		return *this;
	}

	BasicFileMapping(const BasicFileMapping&) = delete;
	BasicFileMapping& operator=(const BasicFileMapping&) = delete;

	index_t Length() const {return mData.Length();}

	void Close();

	bool operator==(decltype(nullptr)) const {return mData == nullptr;}
	bool operator!=(decltype(nullptr)) const {return mData != nullptr;}

protected:
	Span<byte> mData;
	String mFilePath;

	BasicFileMapping() {}
	BasicFileMapping(StringView fileName, uint64 startByte, size_t bytes, bool writeAccess, ErrorReporter err);
	~BasicFileMapping() {Close();}
};

class FileMapping: public BasicFileMapping
{
public:
	FileMapping(StringView fileName, uint64 startByte, size_t bytes, ErrorReporter err):
		BasicFileMapping(fileName, startByte, bytes, false, err) {}

	FileMapping(StringView fileName, ErrorReporter err):
		BasicFileMapping(fileName, 0, ~size_t(0), false, err) {}

	FileMapping(FileMapping&&) = default;
	FileMapping(const FileMapping&) = delete;

	FileMapping& operator=(FileMapping&&) = default;
	FileMapping& operator=(const FileMapping&) = delete;

	const byte* Data() const {return mData.Data();}

	Span<const byte> AsRange() const {return mData;}

	template<typename T> Requires<
		CTriviallySerializable<T>,
	Span<const T>> AsRangeOf() const {return CSpanOfRaw<T>(mData.Data(), size_t(mData.Length()));}
};

class WritableFileMapping: public BasicFileMapping
{
public:
	WritableFileMapping(StringView fileName, uint64 startByte, size_t bytes, ErrorReporter err):
		BasicFileMapping(fileName, startByte, bytes, true, err) {}

	WritableFileMapping(StringView fileName, ErrorReporter err):
		BasicFileMapping(fileName, 0, ~size_t(0), true, err) {}

	WritableFileMapping(WritableFileMapping&&) = default;
	WritableFileMapping(const WritableFileMapping&) = delete;

	WritableFileMapping& operator=(WritableFileMapping&&) = default;
	WritableFileMapping& operator=(const WritableFileMapping&) = delete;

	void Flush();

	byte* Data() const {return mData.Data();}

	Span<byte> AsRange() const {return mData;}

	template<typename T> Requires<
		CTriviallySerializable<T>,
	Span<T>> AsRangeOf() const {return SpanOfRaw<T>(mData.Data(), mData.Length());}
};
} INTRA_END