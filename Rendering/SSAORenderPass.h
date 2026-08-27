#pragma once
#include "RenderPass.h"

class SSAORenderPass : public RenderPass
{
public:
    void Initialize(std::shared_ptr<D3D12Driver> device, int width, int height) override;
    void Pass(std::shared_ptr<D3D12Driver> device, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget) override;
    void OnResize(std::shared_ptr<D3D12Driver> device, int width, int height) override;

    std::shared_ptr<Texture> GetSSAOTexture() { return m_SSAOBlurTexture; }

private:
    static const int KernelSize = 16;
    static const int NoiseDim = 4;

    int m_width = 512;
    int m_height = 512;

    std::shared_ptr<ComputePipeline> m_SSAOPipeline;
    std::shared_ptr<Texture> m_SSAOTexture;
    std::shared_ptr<Buffer> m_sceneConstantBuffer;
    std::shared_ptr<Buffer> m_ssaoParamsBuffer;
    std::shared_ptr<Sampler> m_sampler;
    std::shared_ptr<Buffer> m_kernelBuffer;
    std::shared_ptr<Texture> m_noiseTexture;
    std::shared_ptr<Sampler> m_noiseSampler;

    std::shared_ptr<ComputePipeline> m_SSAOBlurPipeline;
    std::shared_ptr<Texture> m_SSAOBlurTexture;
};
