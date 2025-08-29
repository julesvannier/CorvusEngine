#pragma once
#include "Camera.h"
#include "RenderingLayouts.h"
#include "RenderItem.h"

struct DirectionalLightInfo
{
    DirectX::XMFLOAT3 Direction = { 1.0, 1.0, 0.0 };
    float Intensity = 1.0f;
};

struct ShadowMap
{
    DirectX::XMFLOAT4X4 ShadowTransform = {};
    std::shared_ptr<Texture> DepthBuffer;
};

struct GBuffer
{
    std::shared_ptr<Texture> AlbedoRenderTarget;
    std::shared_ptr<Texture> NormalRenderTarget;
    std::shared_ptr<Texture> MetallicRoughnessRenderTarget;
    std::shared_ptr<Texture> DepthBuffer;
};

struct EnvironmentMaps
{
    std::shared_ptr<TextureCube> SkyBox;
    std::shared_ptr<TextureCube> DiffuseIrradianceMap;
    std::shared_ptr<TextureCube> PrefilterEnvMap;
    std::shared_ptr<Texture> BRDFLut;
};

struct WaterParams
{
    DirectX::XMFLOAT4 WaterColor = { 25.0f/255.0f, 207.0f/255.0f, 237.0f/255.0f, 0.65f };
    float WavesScalar = 0.042f;
    float NormalScrollSpeed = 0.01f;
    float NormalTilingFactor = 3.6f;
    float NormalTilingFactor2 = 1.5f;
};

struct GlobalPassData
{
    float DeltaTime;
    float ElapsedTime;
    int ViewMode;
    std::vector<PointLight> PointLights;
    DirectionalLightInfo DirectionalInfo;
    float ViewportSizeX;
    float ViewportSizeY;
    bool EnableShadows;
    ShadowMap ShadowMap;
    GBuffer GBuffer;
    WaterParams WaterParams;
    EnvironmentMaps EnviroMaps;
};

struct RenderMeshData
{
    std::string MeshIdentifier;
    std::vector<Primitive> Primitives;
    std::vector<DirectX::XMFLOAT4X4> InstancesTransforms;
    Material Material;
    std::shared_ptr<Buffer> InstancesDataBuffer;
};

struct RenderTargetInfo
{
    std::shared_ptr<Texture> RenderTexture;
    std::shared_ptr<Texture> DepthBuffer;
};

class RenderPass
{
public:
    virtual ~RenderPass() = default;
    virtual void Initialize(std::shared_ptr<D3D12Renderer> renderer, int width, int height) = 0;
    virtual void Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget) = 0;
    virtual void OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height) = 0;
};
