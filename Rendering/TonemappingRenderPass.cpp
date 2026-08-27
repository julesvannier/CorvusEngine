#include "TonemappingRenderPass.h"

void TonemappingRenderPass::Initialize(std::shared_ptr<D3D12Driver> device, int width, int height)
{
    Shader tonemapShader;
    ShaderCompiler::CompileShader("Shaders/Tonemapping.hlsl", ShaderType::Compute, tonemapShader);
    m_tonemapPipeline = device->CreateComputePipeline(tonemapShader);

    m_tonemapParamsBuffer = device->CreateBuffer(256, 0, BufferType::Constant, false);
    device->CreateConstantBuffer(m_tonemapParamsBuffer);

    OnResize(device, width, height);
}

void TonemappingRenderPass::OnResize(std::shared_ptr<D3D12Driver> device, int width, int height)
{
    m_displayTexture.reset();

    m_width = width;
    m_height = height;

    m_displayTexture = device->CreateTexture(width, height, TextureFormat::RGBA8, TextureType::Storage, "DisplayTexture");
    device->CreateUnorderedAccessView(m_displayTexture);
    device->CreateShaderResourceView(m_displayTexture);
}

void TonemappingRenderPass::Pass(std::shared_ptr<D3D12Driver> device, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget)
{
    TonemapConstantBuffer tonemapCbuf;
    tonemapCbuf.Exposure = m_exposure;
    tonemapCbuf.Operator = m_tonemapOperator;
    tonemapCbuf.Saturation = m_saturation;
    tonemapCbuf.Hue = DirectX::XMConvertToRadians(m_hue);

    void* data;
    m_tonemapParamsBuffer->Map(0, 0, &data);
    if (data)
    {
        memcpy(data, &tonemapCbuf, sizeof(TonemapConstantBuffer));
        m_tonemapParamsBuffer->Unmap(0, 0);
    }

    auto commandList = device->GetCurrentCommandList();

    commandList->BindComputePipeline(m_tonemapPipeline);
    commandList->ImageBarrier(m_displayTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->BindComputeUnorderedAccessView(m_displayTexture, 0);
    commandList->BindComputeShaderResource(renderTarget.RenderTexture, 1);
    commandList->BindComputeConstantBuffer(m_tonemapParamsBuffer, 2);

    commandList->Dispatch((m_width + 31) / 32, (m_height + 31) / 32, 1);

    commandList->ImageBarrier(m_displayTexture, D3D12_RESOURCE_STATE_GENERIC_READ);
}
