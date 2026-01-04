#pragma once

//-----------------------------------------------------------------------------
// Purpose: Cross-platform memory-mapped file for fast read-only access
//-----------------------------------------------------------------------------
class CMemoryMappedFile
{
public:
	CMemoryMappedFile();
	~CMemoryMappedFile();

	bool Open(const char* const filePath);
	void Close();

	inline const char* GetData() const { return m_data; }
	inline size_t GetSize() const { return m_size; }
	inline bool IsOpen() const { return m_data != nullptr; }

	// Direct access to mapped memory at offset
	inline const char* GetDataAt(size_t offset) const 
	{ 
		return (offset < m_size) ? (m_data + offset) : nullptr; 
	}

private:
	char* m_data;
	size_t m_size;

#ifdef _WIN32
	void* m_fileHandle;
	void* m_mappingHandle;
#else
	int m_fd;
#endif
};
