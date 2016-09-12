﻿#pragma once

#include "Stream.h"
#include "Meta/Type.h"
#include "Core/Errors.h"

namespace Intra { namespace IO { namespace DiskFile {

class CommonFileImpl
{
protected:
	struct FileMapping
	{
		void* hndl=null;
	public:
		FileMapping() {}
		byte* data=null;
		size_t Size=0;
	};
public:
	int GetFileDescriptor() const;
	ulong64 GetFileTime() const;

	CommonFileImpl& operator=(null_t) {close(); return *this;}
	bool operator==(null_t) {return hndl==null;}
	bool operator!=(null_t) {return !operator==(null);}

	String GetName() const {return name;}
	const FileMapping& GetMapping() const {return mapping;}

protected:
	CommonFileImpl(): hndl(null) {}

	CommonFileImpl(StringView fileName, bool readAccess, bool writeAccess, bool append, Error* oError): hndl(null)
	{
		open(fileName, readAccess, writeAccess, append, oError);
	}

	CommonFileImpl(CommonFileImpl&& rhs):
		hndl(rhs.hndl), name(core::move(rhs.name)) {rhs.hndl = null;}

	~CommonFileImpl() {close();}

	void open(StringView fileName, bool readAccess, bool writeAccess, bool append, Error* oError=null);
	void close();

	void* hndl;
	String name;
	mutable FileMapping mapping;

private:
	CommonFileImpl(const CommonFileImpl&) = delete;
	CommonFileImpl& operator=(const CommonFileImpl&) = delete;
};

class Reader: public CommonFileImpl, public IInputStream
{
public:
	Reader() {}

	Reader(Reader&& rhs): CommonFileImpl(core::move(rhs)) {}

	Reader(StringView fileName, Error* oError=null):
		CommonFileImpl(fileName, true, false, false, oError) {}

	void SetPos(ulong64 bytes) override;
	ulong64 GetSize() const override;
	ulong64 GetPos() const override;

	template<typename T> ArrayRange<const T> Map(ulong64 firstByte=0, size_t elements = Meta::NumericLimits<size_t>::Max()) const
	{
		if(elements == Meta::NumericLimits<size_t>::Max())
			elements = size_t((GetSize()-firstByte)/sizeof(T));
		map(firstByte, elements*sizeof(T));
		return {(const T*)mapping.data, elements};
	}

	void Unmap() const;

private:

	void map(ulong64 firstByte, size_t bytes) const;

public:
	bool EndOfStream() const override;
	size_t ReadData(void* data, size_t bytes) override final;

	void UnreadData(const void* src, size_t bytes) override final;

	Reader& operator=(Reader&& rhs)
	{
		hndl = rhs.hndl;
		name = rhs.name;
		mapping = rhs.mapping;
		rhs.hndl = null;
		return *this;
	}

	Reader& operator=(null_t) {close(); return *this;}
	bool operator==(null_t) {return hndl==null;}
	bool operator!=(null_t) {return !operator==(null);}

private:
	Reader(const Reader&) = delete;
	Reader& operator=(const Reader&) = delete;
};

class Writer: public CommonFileImpl, public IOutputStream
{
public:
	Writer() {}
	Writer(StringView fileName, bool append=false, Error* oError=null):
		CommonFileImpl(fileName, false, true, append, oError) {}

	void SetPos(ulong64 bytes) override;
	ulong64 GetSize() const override;
	ulong64 GetPos() const override;

	//Запись
	virtual void WriteData(const void* data, size_t length) override final;

	Writer(Writer&& rhs) {hndl=rhs.hndl; rhs.hndl=null;}
	Writer& operator=(Writer&& rhs) {close(); hndl=rhs.hndl; rhs.hndl=null; return *this;}

	Writer& operator=(null_t) {close(); return *this;}
	bool operator==(null_t) {return hndl==null;}
	bool operator!=(null_t) {return !operator==(null);}

private:
	Writer(const Writer&) = delete;
	Writer& operator=(const Writer&) = delete;
};

String ReadAsString(StringView fileName, bool* fileOpened=null);

template<typename T> Array<T> ReadAsArray(StringView fileName, bool* fileOpened)
{
	Reader file(fileName);
	if(fileOpened!=null) *fileOpened = (file!=null);
	if(file==null) return null;
	const auto bytes = (size_t)file.GetSize();
	Array<T> result;
	result.SetCountUninitialized(bytes/sizeof(T));
	file.ReadData(result.Data(), bytes-bytes%sizeof(T));
	file.Skip(bytes%sizeof(T));
	return result;
}

struct Info
{
	bool Exist;
	ulong64 Size;
	ulong64 LastModified;
};

bool Exists(StringView fileName);
bool Delete(StringView filename);
bool MoveOrRename(StringView oldFilename, StringView newFilename);
Info GetInfo(StringView fileName);
ulong64 GetFileTime(StringView filename);

String NormalizeSlashes(StringView path);
String AddTrailingSlash(StringView path);
StringView RemoveTrailingSlash(StringView path);

inline bool IsPathSeparator(char c) {return c=='/' || c=='\\';}
void SplitPath(StringView fullPath, StringView* oDirectoryPath, StringView* oNameOnly, StringView* oExtension, StringView* oName);
StringView ExtractDirectoryPath(StringView fullPath);
StringView ExtractNameWithoutExtension(StringView fullPath);
StringView ExtractName(StringView fullPath);
StringView ExtractExtension(StringView fullPath);

#ifdef GetCurrentDirectory
#undef GetCurrentDirectory
#endif
String GetCurrentDirectory();

}}}