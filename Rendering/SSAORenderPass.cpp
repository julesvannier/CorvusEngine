#include "SSAORenderPass.h"
#include "Utilities.h"
#include "Image.h"
#include <algorithm>

void SSAORenderPass::Initialize(std::shared_ptr<D3D12Driver> device, int width, int height)
{
    m_sampler = device->CreateSampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR);
    
    Shader SSAOShader;
    ShaderCompiler::CompileShader("Shaders/SSAO.hlsl", ShaderType::Compute, SSAOShader);
    m_SSAOPipeline = device->CreateComputePipeline(SSAOShader);

    Shader SSAOBlurShader;
    ShaderCompiler::CompileShader("Shaders/SSAOBlur.hlsl", ShaderType::Compute, SSAOBlurShader);
    m_SSAOBlurPipeline = device->CreateComputePipeline(SSAOBlurShader);

    m_sceneConstantBuffer = device->CreateBuffer(512, 0, BufferType::Constant, false);
    device->CreateConstantBuffer(m_sceneConstantBuffer);

    m_ssaoParamsBuffer = device->CreateBuffer(256, 0, BufferType::Constant, false);
    device->CreateConstantBuffer(m_ssaoParamsBuffer);

    std::vector<DirectX::XMFLOAT4> kernel(KernelSize);
    for (int i = 0; i < KernelSize; i++)
    {
        DirectX::XMVECTOR sample = DirectX::XMVectorSet(
            Utilities::RandomFloatRange(-1.0f, 1.0f),
            Utilities::RandomFloatRange(-1.0f, 1.0f),
            Utilities::RandomFloatRange(0.0f, 1.0f),
            0.0f);
        sample = DirectX::XMVector3Normalize(sample);

        float scale = (float)i / (float)KernelSize;
        scale = Utilities::Lerp(0.1f, 1.0f, scale * scale);
        sample = DirectX::XMVectorScale(sample, scale);

        DirectX::XMStoreFloat4(&kernel[i], sample);
        kernel[i].w = 0.0f;
    }

    m_kernelBuffer = device->CreateBuffer(KernelSize * sizeof(DirectX::XMFLOAT4), sizeof(DirectX::XMFLOAT4), BufferType::Structured, false);

    void* kernelData;
    m_kernelBuffer->Map(0, 0, &kernelData);
    if (kernelData)
    {
        memcpy(kernelData, kernel.data(), KernelSize * sizeof(DirectX::XMFLOAT4));
        m_kernelBuffer->Unmap(0, 0);
    }

    Image noiseImage;
    noiseImage.Width = NoiseDim;
    noiseImage.Height = NoiseDim;
    noiseImage.Bytes = new char[NoiseDim * NoiseDim * 4];

    for (int i = 0; i < NoiseDim * NoiseDim; i++)
    {
        float x = Utilities::RandomFloatRange(-1.0f, 1.0f);
        float y = Utilities::RandomFloatRange(-1.0f, 1.0f);
        int idx = i * 4;
        noiseImage.Bytes[idx + 0] = (char)(std::clamp(x, -1.0f, 1.0f) * 127.0f);
        noiseImage.Bytes[idx + 1] = (char)(std::clamp(y, -1.0f, 1.0f) * 127.0f);
        noiseImage.Bytes[idx + 2] = 0;
        noiseImage.Bytes[idx + 3] = 127;
    }

    m_noiseTexture = device->CreateTexture(NoiseDim, NoiseDim, TextureFormat::RGBA8SNorm, TextureType::ShaderResource, "SSAONoise");
    device->CreateShaderResourceView(m_noiseTexture);
    device->UploadTextureData(noiseImage, m_noiseTexture);
    device->FlushUploads();

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

    m_SSAOBlurTexture.reset();
    m_SSAOBlurTexture = device->CreateTexture(width, height, TextureFormat::R16Norm, TextureType::Storage, "SSAOBlurTexture");
    device->CreateUnorderedAccessView(m_SSAOBlurTexture);
    device->CreateShaderResourceView(m_SSAOBlurTexture);
}

void SSAORenderPass::Pass(std::shared_ptr<D3D12Driver> device, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget)
{
    auto view = camera.GetViewMatrix();
    auto proj = camera.GetProjMatrix();
    auto invViewProj = camera.GetInvViewProjMatrix();

    SceneConstantBuffer cbuf;
    cbuf.Time = globalPassData.ElapsedTime;
    cbuf.CameraPosition = camera.GetPosition();
    cbuf.Mode = globalPassData.ViewMode;
    cbuf.DirLightDirection = globalPassData.DirectionalInfo.Direction;
    cbuf.DirLightIntensity = globalPassData.DirectionalInfo.Intensity;
    cbuf.ScreenDimensions[0] = globalPassData.ViewportSizeX;
    cbuf.ScreenDimensions[1] = globalPassData.ViewportSizeY;
    DirectX::XMStoreFloat4x4(&cbuf.View, view);
    DirectX::XMStoreFloat4x4(&cbuf.Proj, proj);
    DirectX::XMStoreFloat4x4(&cbuf.InvViewProj, invViewProj);
    cbuf.ShadowTransform = globalPassData.ShadowMap.ShadowTransform;
    cbuf.ShadowEnabled = globalPassData.EnableShadows;
    cbuf.AOEnabled = globalPassData.EnableSSAO;

    void* data;
    m_sceneConstantBuffer->Map(0, 0, &data);
    if (data)
    {
        memcpy(data, &cbuf, sizeof(SceneConstantBuffer));
        m_sceneConstantBuffer->Unmap(0, 0);
    }

    SSAOConstantBuffer ssaoCbuf = globalPassData.SSAOParams;

    void* ssaoData;
    m_ssaoParamsBuffer->Map(0, 0, &ssaoData);
    if (ssaoData)
    {
        memcpy(ssaoData, &ssaoCbuf, sizeof(SSAOConstantBuffer));
        m_ssaoParamsBuffer->Unmap(0, 0);
    }

    auto commandList = device->GetCurrentCommandList();

    commandList->BindComputePipeline(m_SSAOPipeline);
    commandList->ImageBarrier(m_SSAOTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->BindComputeUnorderedAccessView(m_SSAOTexture, 0);
    commandList->BindComputeShaderResource(globalPassData.GBuffer.DepthBuffer, 1);
    commandList->BindComputeSampler(m_sampler, 2);
    commandList->BindComputeConstantBuffer(m_sceneConstantBuffer, 3);
    commandList->BindComputeShaderResource(globalPassData.GBuffer.NormalRenderTarget, 4);
    commandList->BindComputeConstantBuffer(m_ssaoParamsBuffer, 5);
    commandList->SetComputeShaderResource(m_kernelBuffer, 6);
    commandList->BindComputeShaderResource(m_noiseTexture, 7);
    commandList->BindComputeSampler(m_noiseSampler, 8);

    commandList->Dispatch((m_width + 31) / 32, (m_height + 31) / 32, 1);

    commandList->ImageBarrier(m_SSAOTexture, D3D12_RESOURCE_STATE_GENERIC_READ);

    commandList->BindComputePipeline(m_SSAOBlurPipeline);
    commandList->ImageBarrier(m_SSAOBlurTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->BindComputeUnorderedAccessView(m_SSAOBlurTexture, 0);
    commandList->BindComputeShaderResource(m_SSAOTexture, 1);

    commandList->Dispatch((m_width + 31) / 32, (m_height + 31) / 32, 1);

    commandList->ImageBarrier(m_SSAOBlurTexture, D3D12_RESOURCE_STATE_GENERIC_READ);
}