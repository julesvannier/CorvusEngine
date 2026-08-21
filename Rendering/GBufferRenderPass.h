#pragma once
#include "RenderPass.h"

class GBufferRenderPass : public RenderPass
{
public:
    void Initialize(std::shared_ptr<D3D12Driver> device, int width, int height) override;
    void Pass(std::shared_ptr<D3D12Driver> device, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget) override;
    void OnResize(std::shared_ptr<D3D12Driver> device, int width, int height) override;

    GBuffer GetGBuffer() { return m_GBuffer; }

private:
    std::shared_ptr<GraphicsPipeline> m_deferredGeometryPipeline;
    std::shared_ptr<Buffer> m_sceneConstantBuffer;
    std::shared_ptr<Sampler> m_textureSampler;

    GBuffer m_GBuffer;
};
