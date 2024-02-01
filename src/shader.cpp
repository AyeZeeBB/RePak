#include "pch.h"
#include "assets/assets.h"
#include "shader.h"

void Assets::AddShaderSetAsset_stub(CPakFile* pak, std::vector<PakAsset_t>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Error("RPak version 7 (Titanfall 2) Shader Set !!!!NOT IMPLEMENTED!!!!");
}

void Assets::AddShaderSetAsset_v11(CPakFile* pak, std::vector<PakAsset_t>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	std::vector<PakGuidRefHdr_t> guids{};

	Log("\n==============================\n");
	Log("Asset shds -> '%s'\n", assetPath);

	std::string sAssetName = assetPath;

	uint32_t NameDataSize = sAssetName.length() + 1;
	uint32_t NameAlignment = NameDataSize % 4;
	NameDataSize += NameAlignment;
	char* pDataBuf = new char[NameDataSize];

	uint64_t GUID = RTech::StringToGuid(sAssetName.c_str());

	ShaderSetHeader* pHdr = new ShaderSetHeader();

	// Segments
	// asset header
	CPakDataChunk subhdrinfo = pak->CreateDataChunk(sizeof(ShaderSetHeader), SF_HEAD, 16);

	if (mapEntry.HasMember("guid") && mapEntry["guid"].IsUint64())
	{
		GUID = mapEntry["guid"].GetUint64();

		if (GUID == 0)
			Error("Asset '%s' 'GUID' field cannot be 0\n", assetPath);
	}

	bool SaveName = false;
	if (mapEntry.HasMember("savename"))
	{
		if (mapEntry["savename"].IsBool())
			SaveName = mapEntry["savename"].GetBool();
		else Error("Asset '%s' 'savename' field is using invalid type expected 'bool'\n", assetPath);
	}

	// data segment
	CPakDataChunk dataseginfo;
	if (SaveName)
	{
		dataseginfo = pak->CreateDataChunk(NameDataSize, SF_CPU, 64);

		// Write Shader Set Name
		snprintf(pDataBuf, NameDataSize, "%s", sAssetName.c_str());

		pHdr->pName = { dataseginfo.GetIndex(), 0 };
		pak->AddPointer(subhdrinfo.GetIndex(), offsetof(ShaderSetHeader, pName));
	}

	if (mapEntry.HasMember("textures"))
	{
		if (mapEntry["textures"].IsInt())
			pHdr->TextureInputCount = (uint16_t)mapEntry["textures"].GetInt();
		else Error("Asset '%s' 'textures' field is using invalid type expected 'uint'\n", assetPath);
	}
	else Error("Asset '%s' 'textures' field is missing\n", assetPath);

	pHdr->Count1 = pHdr->TextureInputCount;

	if (mapEntry.HasMember("samplers"))
	{
		if (mapEntry["samplers"].IsInt())
			pHdr->NumSamplers = (uint16_t)mapEntry["samplers"].GetInt();
		else Error("Asset '%s' 'samplers' field is using invalid type expected 'uint'\n", assetPath);
	}

	// ensure if texture count is 0 use 0 samplers
	if (pHdr->TextureInputCount == 0)
		pHdr->NumSamplers = 0;

	uint32_t Relations = 0;
	if (mapEntry.HasMember("vertex"))
	{
		if (mapEntry["vertex"].IsString())
			pHdr->VertexShaderGUID = RTech::StringToGuid(mapEntry["vertex"].GetString());
		else if (mapEntry["vertex"].IsUint64())
			pHdr->VertexShaderGUID = mapEntry["vertex"].GetUint64();
		else Error("Asset '%s' 'vertex' field is using invalid type expected 'string , uint64'\n", assetPath);

		if (pHdr->VertexShaderGUID == 0)
			Error("Asset '%s' 'vertex' field cannot be 0\n", assetPath);

		pak->AddGuidDescriptor(&guids, subhdrinfo.GetIndex(), offsetof(ShaderSetHeader, VertexShaderGUID));

		PakAsset_t* shaderAsset = pak->GetAssetByGuid(pHdr->VertexShaderGUID);
		if (shaderAsset)
			shaderAsset->AddRelation(assetEntries->size());
		else
			Log("Couldnt find vertex GUID");

	}
	else Error("Asset '%s' 'vertex' field is missing\n", assetPath);

	if (mapEntry.HasMember("pixel"))
	{
		if (mapEntry["pixel"].IsString())
			pHdr->PixelShaderGUID = RTech::StringToGuid(mapEntry["pixel"].GetString());
		else if (mapEntry["pixel"].IsUint64())
			pHdr->PixelShaderGUID = mapEntry["pixel"].GetUint64();
		else Error("Asset '%s' 'pixel' field is using invalid type expected 'string , uint64'\n", assetPath);

		if (pHdr->PixelShaderGUID == 0)
			Error("Asset '%s' 'pixel' field cannot be 0\n", assetPath);

		pak->AddGuidDescriptor(&guids, subhdrinfo.GetIndex(), offsetof(ShaderSetHeader, PixelShaderGUID));

		PakAsset_t* shaderAsset = pak->GetAssetByGuid(pHdr->PixelShaderGUID);
		if (shaderAsset)
			shaderAsset->AddRelation(assetEntries->size());
		else
			Log("Couldnt find pixel GUID");

	}
	else Error("Asset '%s' 'pixel' field is missing\n", assetPath);

	pak->CreateDataChunk( subhdrinfo.GetIndex(), subhdrinfo.GetSize(), (uint8_t)pHdr);
	uint32_t lastPageIdx = subhdrinfo.GetIndex();

	if (SaveName)
	{
		pak->CreateDataChunk( dataseginfo.GetIndex(), dataseginfo.GetSize(), (uint8_t)pDataBuf);
		lastPageIdx = dataseginfo.GetIndex();
	}


	Log("-> GUID: 0x%llX\n", GUID);

	PakAsset_t asset;

	asset.InitAsset(GUID, subhdrinfo.GetPointer(), subhdrinfo.GetSize(), subhdrinfo.GetPointer(), 0, -1, (std::uint32_t)AssetType::SHDS);
	asset.SetHeaderPointer(subhdrinfo.Data());
	asset.version = 11;

	asset.pageEnd = pak->GetNumPages();
	asset.remainingDependencyCount = 1;

	asset.EnsureUnique(assetEntries);
	assetEntries->push_back(asset);
}

void Assets::AddShaderAsset_v12(CPakFile* pak, std::vector<PakAsset_t>* assetEntries, const char* assetPath, rapidjson::Value& mapEntry)
{
	Log("\n==============================\n");
	Log("Asset shdr -> '%s'\n", assetPath);

	uint64_t GUID = RTech::StringToGuid(assetPath);

	std::string AssetName = assetPath;
	std::string shaderFilePath = pak->GetAssetPath() + AssetName + ".fxc";
	REQUIRE_FILE(shaderFilePath);
	uint32_t ShaderSize = Utils::GetFileSize(shaderFilePath);

	// Read ShaderData
	BinaryIO shdrInput;
	shdrInput.open(shaderFilePath, BinaryIOMode::Read);

	DXBCHeader shdrhdr = shdrInput.read<DXBCHeader>();

	if (shdrhdr.Magic != 0x43425844) // "DXBC"
		Error("invalid file magic for shader asset '%s'. expected %x, found %x\n", assetPath, 0x54534449, shdrhdr.Magic);

	shdrInput.seek(0);

	size_t DataBufferSize = sizeof(ShaderDataHeader) + ShaderSize;
	char* pDataBuf = new char[DataBufferSize];

	// write the shader data into the data buffer
	shdrInput.getReader()->read(pDataBuf + sizeof(ShaderDataHeader), ShaderSize);
	shdrInput.close();

	//while (!IsDebuggerPresent())
	//	::Sleep(100);

	rmem writer(pDataBuf);
	uint64_t offset = sizeof(ShaderDataHeader);

	// go to first chunk and read the header
	writer.seek(offset + 0x34, rseekdir::beg);
	RDefHeader RDefHdr = writer.read<RDefHeader>();

	if (RDefHdr.Magic != 'FEDR')
		Error("invalid RDef magic for shader asset '%s'. expected %x, found %x\n", assetPath, 'FEDR', RDefHdr.Magic);

	if (RDefHdr.MajorVersion >= 5)
		writer.seek(32, rseekdir::cur); // skip RD11 header

	uint32_t Textures = 0;
	if (mapEntry.HasMember("textures") && mapEntry["textures"].IsInt())
		Textures = (uint16_t)mapEntry["textures"].GetInt();

	ShaderHeader* pHdr = new ShaderHeader();

	switch (RDefHdr.ShaderType)
	{
	case ShaderType::PixelShader:
	{
		Log("ShaderType -> Pixel'\n");
		pHdr->ShaderType = HdrShaderType::Pixel;


		if (mapEntry.HasMember("width") && mapEntry["width"].IsNumber())
			pHdr->max_width = mapEntry["width"].GetInt();

		if (mapEntry.HasMember("height") && mapEntry["height"].IsNumber())
			pHdr->max_height = mapEntry["height"].GetInt();

		if (mapEntry.HasMember("minsize") && mapEntry["minsize"].IsNumber())
			pHdr->min_widthheight = mapEntry["minsize"].GetInt();

		Log("-> min dimensions: %ix%i\n", pHdr->min_widthheight, pHdr->min_widthheight);
		Log("-> max dimensions: %ix%i\n", pHdr->max_width, pHdr->max_height);
		break;
	}
	case ShaderType::VertexShader:
	{
		Log("ShaderType -> Vertex'\n");
		pHdr->ShaderType = HdrShaderType::Vertex;

		pHdr->unk = 255;
		pHdr->min_widthheight = 100;

		pHdr->max_width = 0;
		pHdr->max_height = 2;
		break;
	}

	default:
		Error("!!! 'Compute, Domain, Geometry, Hull Shaders are not supported !!!'\n");
		return;
	}

	// Segments
	// asset header
	CPakDataChunk subhdrinfo = pak->CreateDataChunk(sizeof(ShaderHeader), SF_HEAD, 16);

	//_vseginfo_t metadataseginfo = pak->CreateNewSegment((16 * Textures) + (16 * Textures), SF_CPU, 64);
	//char* pMetaDataBuf = new char[(16 * Textures) + (16 * Textures)];
	//rmem metawriter(pMetaDataBuf);

	// data segment
	CPakDataChunk dataseginfo = pak->CreateDataChunk(DataBufferSize, SF_CPU, 64);

	ShaderDataHeader SDRData;
	{
		SDRData.DataSize = ShaderSize;
		SDRData.pData = { dataseginfo.GetIndex(), sizeof(ShaderDataHeader)};
		// write shaderdata
		writer.write(SDRData, 0);

		//pHdr->pIndex0 = { metadataseginfo.index, 0 };
		//pHdr->pTextureSlotData = { metadataseginfo.index, (16 * Textures) };

		//for (int i = 0; i < Textures * 2; i++)
		//{
		//	TextureSlotData Data{};
		//	metawriter.write(Data, (16 * i));
		//}
	}

	pak->AddPointer(subhdrinfo.GetIndex(), offsetof(ShaderHeader, pName));
	//pak->AddPointer(subhdrinfo.index, offsetof(ShaderHeader, pIndex0));
	//pak->AddPointer(subhdrinfo.index, offsetof(ShaderHeader, pTextureSlotData));

	pak->AddPointer(dataseginfo.GetIndex(), offsetof(ShaderDataHeader, pData));

	pak->CreateDataChunk( subhdrinfo.GetIndex(), subhdrinfo.GetSize(), (uint8_t)pHdr);
	//pak->AddRawDataBlock({ metadataseginfo.index, metadataseginfo.size, (uint8_t*)pMetaDataBuf });
	pak->CreateDataChunk( dataseginfo.GetIndex(), dataseginfo.GetSize(), (uint8_t)pDataBuf );

	Log("-> GUID: 0x%llX\n", GUID);

	PakAsset_t asset;

	asset.InitAsset(GUID, subhdrinfo.GetPointer(), subhdrinfo.GetSize(), subhdrinfo.GetPointer(), 0, -1, (std::uint32_t)AssetType::SHDR);
	asset.SetHeaderPointer(subhdrinfo.Data());
	asset.version = 12;

	asset.pageEnd = pak->GetNumPages();
	asset.remainingDependencyCount = 1;

	asset.EnsureUnique(assetEntries);
	assetEntries->push_back(asset);
}