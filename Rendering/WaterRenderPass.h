#pragma once
#include "RenderPass.h"

class WaterRenderPass : public RenderPass
{
public:
    void Initialize(std::shared_ptr<D3D12Renderer> renderer, int width, int height) override;
    void Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTargetInfo) override;
    void OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height) override;

private:
    std::shared_ptr<GraphicsPipeline> m_waterTesselationPipeline;
    std::shared_ptr<Buffer> m_sceneConstantBuffer;
    std::shared_ptr<Buffer> m_waterConstantBuffer;
    std::shared_ptr<Sampler> m_textureSampler;
};
