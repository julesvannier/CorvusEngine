#pragma once
#include "RenderPass.h"

class SSAORenderPass : public RenderPass
{
public:
    void Initialize(std::shared_ptr<D3D12Renderer> renderer, int width, int height) override;
    void Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget) override;
    void OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height) override;

    std::shared_ptr<Texture> GetSSAOTexture() { return m_SSAOTexture; }

private:
    int m_width = 512;
    int m_height = 512;
    
    std::shared_ptr<ComputePipeline> m_SSAOPipeline;
    std::shared_ptr<Texture> m_SSAOTexture;
    std::shared_ptr<Buffer> m_constantBuffer;
    std::shared_ptr<Sampler> m_sampler;
};
