#include "RenderItem.h"

RenderItem::RenderItem()
{
}

RenderItem::~RenderItem()
{
}

void RenderItem::ImportMesh(std::shared_ptr<D3D12Renderer> renderer, std::string filePath)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filePath, aiProcess_FlipWindingOrder | aiProcess_CalcTangentSpace);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        LOG(Error, "RenderItem : Failed assimp import");
        return;
    }
    
    ProcessNode(renderer, scene->mRootNode, scene);
    LOG(Debug, "RenderItem : Imported mesh " + filePath);
    m_path = filePath;
}

void RenderItem::ProcessPrimitive(std::shared_ptr<D3D12Renderer> renderer, aiMesh* mesh, const aiScene* scene)
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

    out.m_vertexBuffer = renderer->CreateBuffer(out.m_vertexCount * sizeof(Vertex), sizeof(Vertex), BufferType::Vertex, false);
    out.m_indicesBuffer = renderer->CreateBuffer(out.m_indexCount * sizeof(uint32_t), 0, BufferType::Index, false);

    Uploader uploader = renderer->CreateUploader();
    uploader.CopyHostToDeviceLocal(vertices.data(), vertices.size() * sizeof(Vertex), out.m_vertexBuffer);
    uploader.CopyHostToDeviceLocal(indices.data(), indices.size() * sizeof(uint32_t), out.m_indicesBuffer);
    renderer->FlushUploader(uploader);

    m_primitives.push_back(out);
}

void RenderItem::ProcessNode(std::shared_ptr<D3D12Renderer> renderer, aiNode* node, const aiScene* scene)
{
    for (int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; // TODO compute transform
        ProcessPrimitive(renderer, mesh, scene);
    }

    for (int i = 0; i < node->mNumChildren; i++)
        ProcessNode(renderer, node->mChildren[i], scene);
}

void RenderItem::CreateQuadMesh(std::shared_ptr<D3D12Renderer> renderer)
{
    Primitive out;
    DirectX::XMMATRIX identityMatrix = DirectX::XMMatrixIdentity();
    DirectX::XMStoreFloat4x4(&out.LocalPrimTransform, identityMatrix);

    std::vector<Vertex> vertices(4);
    std::vector<uint32_t> indices = { 0, 1, 2, 3 };

    // Bottom-left
    vertices[0].Position = DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f);
    vertices[0].UV = DirectX::XMFLOAT2(0.0f, 1.0f);
    vertices[0].Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
    vertices[0].Tangent = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
    vertices[0].Binormal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

    // Bottom-right
    vertices[1].Position = DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f);
    vertices[1].UV = DirectX::XMFLOAT2(1.0f, 1.0f);
    vertices[1].Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
    vertices[1].Tangent = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
    vertices[1].Binormal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

    // Top-right
    vertices[2].Position = DirectX::XMFLOAT3(1.0f, 1.0f, 0.0f);
    vertices[2].UV = DirectX::XMFLOAT2(1.0f, 0.0f);
    vertices[2].Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
    vertices[2].Tangent = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
    vertices[2].Binormal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

    // Top-left
    vertices[3].Position = DirectX::XMFLOAT3(-1.0f, 1.0f, 0.0f);
    vertices[3].UV = DirectX::XMFLOAT2(0.0f, 0.0f);
    vertices[3].Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);
    vertices[3].Tangent = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
    vertices[3].Binormal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

    out.m_vertexCount = vertices.size();
    out.m_indexCount = indices.size();

    out.m_vertexBuffer = renderer->CreateBuffer(out.m_vertexCount * sizeof(Vertex), sizeof(Vertex), BufferType::Vertex, false);
    out.m_indicesBuffer = renderer->CreateBuffer(out.m_indexCount * sizeof(uint32_t), 0, BufferType::Index, false);

    Uploader uploader = renderer->CreateUploader();
    uploader.CopyHostToDeviceLocal(vertices.data(), vertices.size() * sizeof(Vertex), out.m_vertexBuffer);
    uploader.CopyHostToDeviceLocal(indices.data(), indices.size() * sizeof(uint32_t), out.m_indicesBuffer);
    renderer->FlushUploader(uploader);

    m_primitives.push_back(out);
}