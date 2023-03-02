//===============================================================================//
//
// purpose: pakfile system
//
//===============================================================================//

#include "pch.h"
#include "pakfile.h"
#include "application/repak.h"

//-----------------------------------------------------------------------------
// purpose: constructor
//-----------------------------------------------------------------------------
CPakFile::CPakFile(int version)
{
	this->m_Header.fileVersion = version;
	this->m_Version = version;
}

//-----------------------------------------------------------------------------
// purpose: installs asset types and their callbacks
//-----------------------------------------------------------------------------
void CPakFile::AddAsset(rapidjson::Value& file)
{
	ASSET_HANDLER("txtr", file, m_Assets, Assets::AddTextureAsset_v8, Assets::AddTextureAsset_v8);
	ASSET_HANDLER("uimg", file, m_Assets, Assets::AddUIImageAsset_v10, Assets::AddUIImageAsset_v10);
	ASSET_HANDLER("Ptch", file, m_Assets, Assets::AddPatchAsset, Assets::AddPatchAsset);
	ASSET_HANDLER("dtbl", file, m_Assets, Assets::AddDataTableAsset_v0, Assets::AddDataTableAsset_v1);
	ASSET_HANDLER("rmdl", file, m_Assets, Assets::AddModelAsset_stub, Assets::AddModelAsset_v9);
	ASSET_HANDLER("matl", file, m_Assets, Assets::AddMaterialAsset_v12, Assets::AddMaterialAsset_v15);
	ASSET_HANDLER("rseq", file, m_Assets, Assets::AddAnimSeqAsset_stub, Assets::AddAnimSeqAsset_v7);
}

//-----------------------------------------------------------------------------
// purpose: adds page pointer to descriptor
//-----------------------------------------------------------------------------
void CPakFile::AddPointer(unsigned int pageIdx, unsigned int pageOffset)
{
	m_vDescriptors.push_back({ pageIdx, pageOffset });
}

//-----------------------------------------------------------------------------
// purpose: adds guid descriptor
//-----------------------------------------------------------------------------
void CPakFile::AddGuidDescriptor(std::vector<RPakGuidDescriptor>* guids, unsigned int idx, unsigned int offset)
{
	guids->push_back({ idx, offset });
}

//-----------------------------------------------------------------------------
// purpose: adds raw data block
//-----------------------------------------------------------------------------
void CPakFile::AddRawDataBlock(RPakRawDataBlock block)
{
	m_vRawDataBlocks.push_back(block);
};

//-----------------------------------------------------------------------------
// purpose: adds new starpak file path to be used by the rpak
//-----------------------------------------------------------------------------
void CPakFile::AddStarpakReference(const std::string& path)
{
	for (auto& it : m_vStarpakPaths)
	{
		if (it == path)
			return;
	}
	m_vStarpakPaths.push_back(path);
}

//-----------------------------------------------------------------------------
// purpose: adds new optional starpak file path to be used by the rpak
//-----------------------------------------------------------------------------
void CPakFile::AddOptStarpakReference(const std::string& path)
{
	for (auto& it : m_vOptStarpakPaths)
	{
		if (it == path)
			return;
	}
	m_vOptStarpakPaths.push_back(path);
}

//-----------------------------------------------------------------------------
// purpose: adds new starpak data entry
// returns: starpak data entry descriptor
//-----------------------------------------------------------------------------
SRPkDataEntry CPakFile::AddStarpakDataEntry(SRPkDataEntry block)
{
	// starpak data is aligned to 4096 bytes
	size_t ns = Utils::PadBuffer((char**)&block.m_nDataPtr, block.m_nDataSize, 4096);

	block.m_nDataSize = ns;
	block.m_nOffset = m_NextStarpakOffset;

	m_vStarpakDataBlocks.push_back(block);

	m_NextStarpakOffset += block.m_nDataSize;

	return block;
}

//-----------------------------------------------------------------------------
// purpose: writes header to file stream
//-----------------------------------------------------------------------------
void CPakFile::WriteHeader(BinaryIO* io)
{
	m_Header.virtualSegmentCount = m_vVirtualSegments.size();
	m_Header.pageCount = m_vPages.size();
	m_Header.descriptorCount = m_vDescriptors.size();
	m_Header.guidDescriptorCount = m_vGuidDescriptors.size();
	m_Header.relationCount = m_vFileRelations.size();

	int version = m_Header.fileVersion;

	io->write(m_Header.magic);
	io->write(m_Header.fileVersion);
	io->write(m_Header.flags);
	io->write(m_Header.fileTime);
	io->write(m_Header.unk0);
	io->write(m_Header.compressedSize);

	if (version == 8)
		io->write(m_Header.embeddedStarpakOffset);

	io->write(m_Header.unk1);
	io->write(m_Header.decompressedSize);

	if (version == 8)
		io->write(m_Header.embeddedStarpakSize);

	io->write(m_Header.unk2);
	io->write(m_Header.starpakPathsSize);

	if (version == 8)
		io->write(m_Header.optStarpakPathsSize);

	io->write(m_Header.virtualSegmentCount);
	io->write(m_Header.pageCount);
	io->write(m_Header.patchIndex);

	if (version == 8)
		io->write(m_Header.alignment);

	io->write(m_Header.descriptorCount);
	io->write(m_Header.assetCount);
	io->write(m_Header.guidDescriptorCount);
	io->write(m_Header.relationCount);

	if (version == 7)
	{
		io->write(m_Header.unk7count);
		io->write(m_Header.unk8count);
	}
	else if (version == 8)
		io->write(m_Header.unk3);
}

//-----------------------------------------------------------------------------
// purpose: writes assets to file stream
//-----------------------------------------------------------------------------
void CPakFile::WriteAssets(BinaryIO* io)
{
	for (auto& it : m_Assets)
	{
		io->write(it.guid);
		io->write(it.unk0);
		io->write(it.headIdx);
		io->write(it.headOffset);
		io->write(it.cpuIdx);
		io->write(it.cpuOffset);
		io->write(it.starpakOffset);

		if (this->m_Version == 8)
			io->write(it.optStarpakOffset);

		io->write(it.pageEnd);
		io->write(it.unk1);
		io->write(it.relStartIdx);
		io->write(it.usesStartIdx);
		io->write(it.relationCount);
		io->write(it.usesCount);
		io->write(it.headDataSize);
		io->write(it.version);
		io->write(it.id);
	}

	// update header asset count with the assets we've just written
	this->m_Header.assetCount = m_Assets.size();
}

//-----------------------------------------------------------------------------
// purpose: writes raw data blocks to file stream
//-----------------------------------------------------------------------------
void CPakFile::WriteRPakRawDataBlocks(BinaryIO& out)
{
	for (auto it = m_vRawDataBlocks.begin(); it != m_vRawDataBlocks.end(); ++it)
	{
		out.getWriter()->write((char*)it->m_nDataPtr, it->m_nDataSize);
	}
}

//-----------------------------------------------------------------------------
// purpose: populates file relations vector with combined asset relation data
//-----------------------------------------------------------------------------
void CPakFile::GenerateFileRelations()
{
	for (auto& it : m_Assets)
	{
		it.relationCount = it._relations.size();
		it.relStartIdx = m_vFileRelations.size();

		for (int i = 0; i < it._relations.size(); ++i)
			m_vFileRelations.push_back(it._relations[i]);
	}
	m_Header.relationCount = m_vFileRelations.size();
}

//-----------------------------------------------------------------------------
// purpose: 
//-----------------------------------------------------------------------------
void CPakFile::GenerateGuidData()
{
	for (auto& it : m_Assets)
	{
		it.usesCount = it._guids.size();
		it.usesStartIdx = it.usesCount == 0 ? 0 : m_vGuidDescriptors.size();

		for (int i = 0; i < it._guids.size(); ++i)
			m_vGuidDescriptors.push_back({ it._guids[i] });
	}
	m_Header.guidDescriptorCount = m_vGuidDescriptors.size();
}

//-----------------------------------------------------------------------------
// purpose: 
// returns: 
//-----------------------------------------------------------------------------
_vseginfo_t CPakFile::CreateNewSegment(uint32_t size, uint32_t flags, uint32_t alignment, uint32_t vsegAlignment /*= -1*/)
{
	uint32_t vsegidx = (uint32_t)m_vVirtualSegments.size();

	// find existing "segment" with the same values or create a new one, this is to overcome the engine's limit of having max 20 of these
	// since otherwise we write into unintended parts of the stack, and that's bad
	RPakVirtualSegment seg = GetMatchingSegment(flags, vsegAlignment == -1 ? alignment : vsegAlignment, &vsegidx);

	bool bShouldAddVSeg = seg.dataSize == 0;
	seg.dataSize += size;

	if (bShouldAddVSeg)
		m_vVirtualSegments.emplace_back(seg);
	else
		m_vVirtualSegments[vsegidx] = seg;

	RPakPageInfo vsegblock{ vsegidx, alignment, size };

	m_vPages.emplace_back(vsegblock);
	unsigned int pageidx = m_vPages.size() - 1;

	return { pageidx, size };
}

//-----------------------------------------------------------------------------
// purpose: 
// returns: 
//-----------------------------------------------------------------------------
RPakAssetEntry* CPakFile::GetAssetByGuid(uint64_t guid, uint32_t* idx /*= nullptr*/)
{
	uint32_t i = 0;
	for (auto& it : m_Assets)
	{
		if (it.guid == guid)
		{
			if (idx)
				*idx = i;
			return &it;
		}
		i++;
	}
	Debug("failed to find asset with guid %llX\n", guid);
	return nullptr;
}

//-----------------------------------------------------------------------------
// purpose: creates page and segment with the specified parameters
// returns: 
//-----------------------------------------------------------------------------
RPakVirtualSegment CPakFile::GetMatchingSegment(uint32_t flags, uint32_t alignment, uint32_t* segidx)
{
	uint32_t i = 0;
	for (auto& it : m_vVirtualSegments)
	{
		if (it.flags == flags && it.alignment == alignment)
		{
			*segidx = i;
			return it;
		}
		i++;
	}

	return { flags, alignment, 0 };
}
