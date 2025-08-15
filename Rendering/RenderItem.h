#pragma once
#include "Core.h"
#include "../RHI/D3D12Renderer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

struct Material
{
    bool HasAlbedo = false;
    std::shared_ptr<Texture> Albedo;
    bool HasNormal = false;
    std::shared_ptr<Texture> Normal;
    bool HasMetallicRoughness = false;
    std::shared_ptr<Texture> MetallicRoughness;
    bool IsTransparent = false;
    float Opacity = 0.5f;
    bool CastShadows = true;
};

struct Vertex
{
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 UV;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 Tangent;
    DirectX::XMFLOAT3 Binormal;
};

struct Primitive
{
    DirectX::XMFLOAT4X4 LocalPrimTransform; // Local Prim Transform, in Object Space
    std::shared_ptr<Buffer> m_vertexBuffer;
    std::shared_ptr<Buffer> m_indicesBuffer;
    int m_vertexCount;
    int m_indexCount;
};

class RenderItem
{
public:
    RenderItem();
    ~RenderItem();
    
    void ImportMesh(std::shared_ptr<D3D12Renderer> renderer, std::string filePath);
    void CreateQuadMesh(std::shared_ptr<D3D12Renderer> renderer);
    void SetPath(const std::string& filePath) { m_path = filePath; }

    std::string GetPath() { return m_path; }
    std::vector<Primitive>& GetPrimitives() { return m_primitives; }
    Material& GetMaterial() { return m_material; }
    std::string GetMeshIdentifier() { return m_path; }
    
private:
    void ProcessPrimitive(std::shared_ptr<D3D12Renderer> renderer, aiMesh *mesh, const aiScene *scene);
    void ProcessNode(std::shared_ptr<D3D12Renderer> renderer, aiNode *node, const aiScene *scene);

    std::string m_path;
    std::vector<Primitive> m_primitives;
    Material m_material;
    
    int m_instanceCount = 1;
};
