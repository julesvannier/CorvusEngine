#include "RenderItem.h"

RenderItem::RenderItem()
{
}

RenderItem::~RenderItem()
{
}

void RenderItem::ImportMesh(std::shared_ptr<D3D12Driver> device, std::string filePath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath, aiProcess_FlipWindingOrder | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG(Error, "RenderItem : Failed assimp import");
        return;
    }
    
    ProcessNode(device, scene->mRootNode, scene);
    LOG(Debug, "RenderItem : Imported mesh " + filePath);
    m_path = filePath;
}

void RenderItem::ProcessPrimitive(std::shared_ptr<D3D12Driver> device, aiMesh* mesh, const aiScene* scene)
{
    Primitive out;
    DirectX::XMMATRIX identityMatrix = DirectX::XMMatrixIdentity();
    DirectX::XMStoreFloat4x4(&out.LocalPrimTransform, identityMatrix);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    for (int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        vertex.Position = DirectX::XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        if (mesh->HasNormals())
        {
            vertex.Normal = DirectX::XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            vertex.Tangent = DirectX::XMFLOAT3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.Binormal = DirectX::XMFLOAT3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        
        if (mesh->mTextureCoords[0])
            vertex.UV = DirectX::XMFLOAT2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        
        vertices.push_back(vertex);
    }

    for (int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    out.m_vertexCount = vertices.size();
    out.m_indexCount = indices.size();

    out.m_vertexBuffer = device->CreateBuffer(out.m_vertexCount * sizeof(Vertex), sizeof(Vertex), BufferType::Vertex, false);
    out.m_indicesBuffer = device->CreateBuffer(out.m_indexCount * sizeof(uint32_t), 0, BufferType::Index, false);

    device->UploadBufferData(vertices.data(), vertices.size() * sizeof(Vertex), out.m_vertexBuffer);
    device->UploadBufferData(indices.data(), indices.size() * sizeof(uint32_t), out.m_indicesBuffer);
    device->FlushUploads();

    m_primitives.push_back(out);
}

void RenderItem::ProcessNode(std::shared_ptr<D3D12Driver> device, aiNode* node, const aiScene* scene)
{
    for (int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; // TODO compute transform
        ProcessPrimitive(device, mesh, scene);
    }

    for (int i = 0; i < node->mNumChildren; i++)
        ProcessNode(device, node->mChildren[i], scene);
}

void RenderItem::CreateQuadMesh(std::shared_ptr<D3D12Driver> device)
{
    Primitive out;
    DirectX::XMMATRIX identityMatrix = DirectX::XMMatrixIdentity();
    DirectX::XMStoreFloat4x4(&out.LocalPrimTransform, identityMatrix);

    std::vector<LightVertex> vertices(4);
    std::vector<uint32_t> indices = { 0, 1, 2, 3 };

    vertices[0].Position = DirectX::XMFLOAT3(-10.0f, 0.0f, 10.0f);
    vertices[1].Position = DirectX::XMFLOAT3(10.0f, 0.0f, 10.0f);
    vertices[2].Position = DirectX::XMFLOAT3(-10.0f, 0.0f, -10.0f);
    vertices[3].Position = DirectX::XMFLOAT3(10.0f, 0.0f, -10.0f);

    out.m_vertexCount = vertices.size();
    out.m_indexCount = indices.size();

    out.m_vertexBuffer = device->CreateBuffer(out.m_vertexCount * sizeof(LightVertex), sizeof(LightVertex), BufferType::Vertex, false);
    out.m_indicesBuffer = device->CreateBuffer(out.m_indexCount * sizeof(uint32_t), 0, BufferType::Index, false);

    device->UploadBufferData(vertices.data(), vertices.size() * sizeof(LightVertex), out.m_vertexBuffer);
    device->UploadBufferData(indices.data(), indices.size() * sizeof(uint32_t), out.m_indicesBuffer);
    device->FlushUploads();

    m_primitives.push_back(out);
}