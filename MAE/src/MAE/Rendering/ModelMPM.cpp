#include <MAE/Rendering/ModelMPM.h>

#include <MAE/Main.h>
#include <MAE/Core/Utils.h>

#include <fstream>

MPMModel::~MPMModel() {
	for (auto i : meshes) {
		if (i.decl != 0)
			i.decl->Release();
		
		if (i.vb != 0)
			i.vb->Release();

		if (i.ib != 0)
			i.ib->Release();
	}

	meshes.clear();
	instances.clear();
}

void MPMModel::load(const std::string& model) {
	std::ifstream f(model, std::ios::in | std::ios::binary);

	MPM::Header h;
	
	if (!readFromStream(f, h))
		throw new std::exception("Failed to read from file");

	if (h.magicNumber != MPM::MagicNumber)
		throw new std::exception("Invalid header");

	if (h.compVersion > MPM::Version)
		throw new std::exception("Invalid header");

	for (uint i = 0; i < h.numMeshes; ++i)
		meshes.push_back({0, 0, 0, 0, 0, 0, 0});

	MPM::PacketHeader ph;

	if (!readFromStream(f, ph))
		throw new std::exception("Failed to read from file");

	while (ph.id != MPM::PacketEndOfFileID) {
		auto offs = f.tellg();

		switch (ph.id) {
		case MPM::PacketInstID:
			readInstances(f);
			break;
		case MPM::PacketMeshID:
			readMesh(f);
			break;
		case MPM::PacketVertexDescID:
			readVertexDesc(f);
			break;
		case MPM::PacketVertexDataID:
			readVertexData(f);
			break;
		case MPM::PacketVertexIndexDataID:
			readIndexData(f);
			break;
		default:
			f.seekg(ph.length, std::ios_base::cur);
		}

		if (f.tellg() - offs != ph.length)
			throw new std::exception("Failed to read from file");

		if (!readFromStream(f, ph))
			throw new std::exception("Failed to read from file");
	}
}

void MPMModel::readInstances(std::ifstream& f) {
	MPM::InstHeader ih;

	if (!readFromStream(f, ih))
		throw new std::exception("Failed to read from file");

	std::vector<MPM::Inst> inst;

	inst.reserve(ih.num);

	for (uint i = 0; i < ih.num; ++i) {
		MPM::Inst in;

		if (!readFromStream(f, in))
			throw new std::exception("Failed to read from file");

		inst.push_back(in);
	}

	instances.reserve(ih.num);

	for (auto i : inst)
		instances.push_back({i.meshInd, mat4::compose(i.scal, i.rot, i.trans)});
}

void MPMModel::readMesh(std::ifstream& f) {
	MPM::Mesh mesh;

	if (!readFromStream(f, mesh))
		throw new std::exception("Failed to read from file");

	meshes[mesh.meshId].matInd      = mesh.matInd;
	meshes[mesh.meshId].numPrim     = (mesh.numIndices == 0 ? mesh.numVertices : mesh.numIndices) / 3;
	meshes[mesh.meshId].numVertices = mesh.numVertices;
}

void MPMModel::readVertexDesc(std::ifstream& f) {
	MPM::PacketVertexDescHeader vh;

	if (!readFromStream(f, vh))
		throw new std::exception("Failed to read from file");

	meshes[vh.meshInd].stride = vh.vertStride;

	std::vector<MPM::PacketVertexDesc> vertexDescs;

	vertexDescs.reserve(vh.num);

	for (uint i = 0; i < vh.num; ++i) {
		MPM::PacketVertexDesc vd;

		if (!readFromStream(f, vd))
			throw new std::exception("Failed to read from file");

		vertexDescs.push_back(vd);
	}

	std::vector<D3DVERTEXELEMENT9> elements;
	elements.reserve(vh.num + 1);

	BYTE typeTable[] = {
		D3DDECLTYPE_UNUSED,
		D3DDECLTYPE_FLOAT1,
		D3DDECLTYPE_FLOAT2,
		D3DDECLTYPE_FLOAT3,
		D3DDECLTYPE_FLOAT4,
		D3DDECLTYPE_D3DCOLOR,
		D3DDECLTYPE_UBYTE4,
		D3DDECLTYPE_SHORT2,
		D3DDECLTYPE_SHORT4
	};

	BYTE usageTable[] = {
		D3DDECLUSAGE_POSITION,
		D3DDECLUSAGE_POSITION,
		D3DDECLUSAGE_NORMAL,
		D3DDECLUSAGE_TEXCOORD,
		D3DDECLUSAGE_COLOR
	};

	for (auto i : vertexDescs)
		elements.push_back({0, i.offset, typeTable[i.type], 0, usageTable[i.usage], 0});

	elements.push_back(D3DDECL_END());

	HRESULT res = mainObj->d3ddev->CreateVertexDeclaration(elements.data(), &meshes[vh.meshInd].decl);

	if (FAILED(res))
		throw new std::exception("Failed to create VertexDeclaration");
}

void MPMModel::readVertexData(std::ifstream& f) {
	MPM::PacketVertexDataHeader vdh;

	if (!readFromStream(f, vdh))
		throw new std::exception("Failed to read from file");

	LPDIRECT3DVERTEXBUFFER9 vb;

	HRESULT res = mainObj->d3ddev->CreateVertexBuffer(vdh.length, 0, 0, D3DPOOL_DEFAULT, &vb, 0);

	if (FAILED(res))
		throw new std::exception("Failed to create VertexBuffer");

	void* data;

	vb->Lock(0, 0, &data, 0);

	f.read((char*) data, vdh.length);

	vb->Unlock();

	meshes[vdh.meshInd].vb = vb;
}

void MPMModel::readIndexData(std::ifstream& f) {
	MPM::PacketVertexIndexHeader vih;

	if (!readFromStream(f, vih))
		throw new std::exception("Failed to read from file");

	uint length = vih.num * (vih.type == vih.TypeU32 ? sizeof(uint) : sizeof(ushort));

	LPDIRECT3DINDEXBUFFER9 ib;

	HRESULT res = mainObj->d3ddev->CreateIndexBuffer(length, 0, vih.type == vih.TypeU32 ? D3DFMT_INDEX32 : D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, 0);

	if (FAILED(res))
		throw new std::exception("Failed to create IndexBuffer");

	void* data;

	ib->Lock(0, 0, &data, 0);

	f.read((char*) data, length);

	ib->Unlock();

	meshes[vih.meshInd].ib = ib;
}

void MPMModel::render() {
	for (auto i : instances) {
		auto& mesh = meshes[i.meshInd];

		mainObj->d3ddev->SetTransform(D3DTS_WORLDMATRIX(1), (D3DMATRIX*) &i.transform.data);

		mainObj->d3ddev->SetVertexDeclaration(mesh.decl);
		mainObj->d3ddev->SetStreamSource(0, mesh.vb, 0, mesh.stride);

		if (mesh.ib == 0)
			mainObj->d3ddev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, mesh.numPrim);
		else {
			mainObj->d3ddev->SetIndices(mesh.ib);
			mainObj->d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, mesh.numVertices, 0, mesh.numPrim);
		}
	}

	mat4 m;
	mainObj->d3ddev->SetTransform(D3DTS_WORLDMATRIX(1), (D3DMATRIX*) &m.data);
}
