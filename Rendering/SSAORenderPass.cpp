#include "SSAORenderPass.h"
#include "Utilities.h"
#include "Image.h"

void SSAORenderPass::Initialize(std::shared_ptr<D3D12Driver> device, int width, int height)
{
    m_sampler = device->CreateSampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR);
    
    Shader SSAOShader;
    ShaderCompiler::CompileShader("Shaders/SSAO.hlsl", ShaderType::Compute, SSAOShader);
    m_SSAOPipeline = device->CreateComputePipeline(SSAOShader);

    m_constantBuffer = device->CreateBuffer(256, 0, BufferType::Constant, false);
    device->CreateConstantBuffer(m_constantBuffer);

    m_noiseSampler = device->CreateSampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MIN_MAG_MIP_POINT);

    OnResize(device, width, height);
}

void SSAORenderPass::OnResize(std::shared_ptr<D3D12Driver> device, int width, int height)
{
    m_SSAOTexture.reset();

    m_width = width;
    m_height = height;
    
    m_SSAOTexture = device->CreateTexture(width, height, TextureFormat::R16Norm, TextureType::Storage, "SSAOTexture");
    device->CreateUnorderedAccessView(m_SSAOTexture);
    device->CreateShaderResourceView(m_SSAOTexture);
}

void SSAORenderPass::Pass(std::shared_ptr<D3D12Driver> device, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget)
{
    SSAOConstantBuffer constantBuffer;
    constantBuffer.Mode = globalPassData.ViewMode;

    void* data;
    m_constantBuffer->Map(0, 0, &data);
    if (data)
    {
        memcpy(data, &constantBuffer, sizeof(SSAOConstantBuffer));
        m_constantBuffer->Unmap(0, 0);
    }

    auto commandList = device->GetCurrentCommandList();

    commandList->BindComputePipeline(m_SSAOPipeline);
    commandList->ImageBarrier(m_SSAOTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->BindComputeUnorderedAccessView(m_SSAOTexture, 0);
    commandList->BindComputeShaderResource(globalPassData.GBuffer.DepthBuffer, 1);
    commandList->BindComputeSampler(m_sampler, 2);
    commandList->BindComputeConstantBuffer(m_constantBuffer, 3);
    commandList->BindComputeShaderResource(globalPassData.GBuffer.NormalRenderTarget, 4);
    
    commandList->Dispatch(m_width / 32, m_height / 32, 6);
    
    commandList->ImageBarrier(m_SSAOTexture, D3D12_RESOURCE_STATE_GENERIC_READ);
}