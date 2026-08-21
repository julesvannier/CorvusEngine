#pragma once
#include "RenderPass.h"

class WaterRenderPass : public RenderPass
{
public:
    void Initialize(std::shared_ptr<D3D12Driver> device, int width, int height) override;
    void Pass(std::shared_ptr<D3D12Driver> device, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTargetInfo) override;
    void OnResize(std::shared_ptr<D3D12Driver> device, int width, int height) override;

private:
    std::shared_ptr<GraphicsPipeline> m_waterTesselationPipeline;
    std::shared_ptr<Buffer> m_sceneConstantBuffer;
    std::shared_ptr<Buffer> m_waterConstantBuffer;
    std::shared_ptr<Sampler> m_textureSampler;
    std::shared_ptr<Texture> m_normalMap;
    std::shared_ptr<Texture> m_normalMap2;
    std::shared_ptr<Texture> m_noiseTex;
};
