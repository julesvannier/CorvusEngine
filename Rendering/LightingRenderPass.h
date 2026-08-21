#pragma once
#include "RenderPass.h"

class LightingRenderPass : public RenderPass
{
public:
    void Initialize(std::shared_ptr<D3D12Driver> device, int width, int height) override;
    void Pass(std::shared_ptr<D3D12Driver> device, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget) override;
    void OnResize(std::shared_ptr<D3D12Driver> device, int width, int height) override;

private:
    std::shared_ptr<GraphicsPipeline> m_deferredDirLightPipeline;
    std::shared_ptr<GraphicsPipeline> m_deferredPointLightPipeline;
    std::shared_ptr<Buffer> m_sceneConstantBuffer;
    std::shared_ptr<Sampler> m_textureSampler;
    std::shared_ptr<Sampler> m_comparisonSampler;
    std::shared_ptr<RenderItem> m_pointLightMesh;
    std::shared_ptr<Buffer> m_instancedLightsInstanceDataTransformBuffer;
    std::shared_ptr<Buffer> m_instancedLightsInstanceDataInfoBuffer;
};
