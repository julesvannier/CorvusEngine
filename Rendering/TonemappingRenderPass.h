#pragma once
#include "RenderPass.h"

class TonemappingRenderPass : public RenderPass
{
public:
    void Initialize(std::shared_ptr<D3D12Driver> device, int width, int height) override;
    void Pass(std::shared_ptr<D3D12Driver> device, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget) override;
    void OnResize(std::shared_ptr<D3D12Driver> device, int width, int height) override;

    std::shared_ptr<Texture> GetDisplayTexture() { return m_displayTexture; }

    float m_exposure = 1.0f;

private:
    int m_width = 0;
    int m_height = 0;

    std::shared_ptr<ComputePipeline> m_tonemapPipeline;
    std::shared_ptr<Texture> m_displayTexture;
    std::shared_ptr<Buffer> m_tonemapParamsBuffer;
};
