//=============================================================================//
//
// Cross-platform memory-mapped file implementation
//
//=============================================================================//
#include <pch.h>
#include "memmap.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CMemoryMappedFile::CMemoryMappedFile()
	: m_data(nullptr)
	, m_size(0)
#ifdef _WIN32
	, m_fileHandle(INVALID_HANDLE_VALUE)
	, m_mappingHandle(nullptr)
#else
	, m_fd(-1)
#endif
{
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CMemoryMappedFile::~CMemoryMappedFile()
{
	Close();
}

//-----------------------------------------------------------------------------
// Purpose: opens and memory-maps a file for reading
//-----------------------------------------------------------------------------
bool CMemoryMappedFile::Open(const char* const filePath)
{
	Close(); // Close any existing mapping

#ifdef _WIN32
	m_fileHandle = CreateFileA(
		filePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN, // Hint to OS for sequential access
		nullptr
	);

	if (m_fileHandle == INVALID_HANDLE_VALUE)
		return false;

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(m_fileHandle, &fileSize))
	{
		CloseHandle(m_fileHandle);
		m_fileHandle = INVALID_HANDLE_VALUE;
		return false;
	}

	m_size = static_cast<size_t>(fileSize.QuadPart);

	if (m_size == 0)
	{
		CloseHandle(m_fileHandle);
		m_fileHandle = INVALID_HANDLE_VALUE;
		return false;
	}

	m_mappingHandle = CreateFileMappingA(
		m_fileHandle,
		nullptr,
		PAGE_READONLY,
		0,
		0,
		nullptr
	);

	if (!m_mappingHandle)
	{
		CloseHandle(m_fileHandle);
		m_fileHandle = INVALID_HANDLE_VALUE;
		return false;
	}

	m_data = static_cast<char*>(MapViewOfFile(
		m_mappingHandle,
		FILE_MAP_READ,
		0,
		0,
		0
	));

	if (!m_data)
	{
		CloseHandle(m_mappingHandle);
		CloseHandle(m_fileHandle);
		m_mappingHandle = nullptr;
		m_fileHandle = INVALID_HANDLE_VALUE;
		return false;
	}

#else
	m_fd = open(filePath, O_RDONLY);
	if (m_fd < 0)
		return false;

	struct stat sb;
	if (fstat(m_fd, &sb) < 0)
	{
		close(m_fd);
		m_fd = -1;
		return false;
	}

	m_size = static_cast<size_t>(sb.st_size);

	if (m_size == 0)
	{
		close(m_fd);
		m_fd = -1;
		return false;
	}

	m_data = static_cast<char*>(mmap(
		nullptr,
		m_size,
		PROT_READ,
		MAP_PRIVATE,
		m_fd,
		0
	));

	if (m_data == MAP_FAILED)
	{
		m_data = nullptr;
		close(m_fd);
		m_fd = -1;
		return false;
	}

	// Advise kernel that we'll read sequentially for optimal prefetching
	madvise(m_data, m_size, MADV_SEQUENTIAL);
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: closes and unmaps the file
//-----------------------------------------------------------------------------
void CMemoryMappedFile::Close()
{
#ifdef _WIN32
	if (m_data)
	{
		UnmapViewOfFile(m_data);
		m_data = nullptr;
	}

	if (m_mappingHandle)
	{
		CloseHandle(m_mappingHandle);
		m_mappingHandle = nullptr;
	}

	if (m_fileHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_fileHandle);
		m_fileHandle = INVALID_HANDLE_VALUE;
	}
#else
	if (m_data)
	{
		munmap(m_data, m_size);
		m_data = nullptr;
	}

	if (m_fd >= 0)
	{
		close(m_fd);
		m_fd = -1;
	}
#endif

	m_size = 0;
}
