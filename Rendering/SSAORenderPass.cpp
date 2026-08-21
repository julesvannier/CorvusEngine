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

    GenerateKernel(device);
    GenerateNoiseTexture(device);

    OnResize(device, width, height);
}

void SSAORenderPass::GenerateKernel(std::shared_ptr<D3D12Driver> device)
{
    std::vector<DirectX::XMFLOAT4> kernel;
    kernel.reserve(KernelSize);

    for (int i = 0; i < KernelSize; i++)
    {
        DirectX::XMVECTOR sample = DirectX::XMVectorSet(
            Utilities::RandomFloatRange(-1.0f, 1.0f),
            Utilities::RandomFloatRange(-1.0f, 1.0f),
            Utilities::RandomFloatRange(0.0f, 1.0f),
            0.0f);

        sample = DirectX::XMVector3Normalize(sample);
        sample = DirectX::XMVectorScale(sample, Utilities::RandomFloatRange(0.0f, 1.0f));

        float scale = (float)i / (float)KernelSize;
        scale = Utilities::Lerp(0.1f, 1.0f, scale * scale);
        sample = DirectX::XMVectorScale(sample, scale);

        DirectX::XMFLOAT4 sampleF4;
        DirectX::XMStoreFloat4(&sampleF4, sample);
        kernel.push_back(sampleF4);
    }

    m_kernelBuffer = device->CreateBuffer(KernelSize * sizeof(DirectX::XMFLOAT4), sizeof(DirectX::XMFLOAT4), BufferType::Structured, false);
    device->UploadBufferData(kernel.data(), KernelSize * sizeof(DirectX::XMFLOAT4), m_kernelBuffer);
    device->FlushUploads();
}

void SSAORenderPass::GenerateNoiseTexture(std::shared_ptr<D3D12Driver> device)
{
    Image noiseImg;
    noiseImg.Width = NoiseDim;
    noiseImg.Height = NoiseDim;
    noiseImg.Bytes = new char[NoiseDim * NoiseDim * 4];

    for (int i = 0; i < NoiseDim * NoiseDim; i++)
    {
        int8_t x = (int8_t)(Utilities::RandomFloatRange(-1.0f, 1.0f) * 127.0f);
        int8_t y = (int8_t)(Utilities::RandomFloatRange(-1.0f, 1.0f) * 127.0f);

        noiseImg.Bytes[i * 4 + 0] = (char)x;
        noiseImg.Bytes[i * 4 + 1] = (char)y;
        noiseImg.Bytes[i * 4 + 2] = 0;
        noiseImg.Bytes[i * 4 + 3] = 0;
    }

    m_noiseTexture = device->CreateTexture(NoiseDim, NoiseDim, TextureFormat::RGBA8SNorm, TextureType::ShaderResource);
    device->CreateShaderResourceView(m_noiseTexture);
    device->UploadTextureData(noiseImg, m_noiseTexture);
    device->FlushUploads();
}

void SSAORenderPass::OnResize(std::shared_ptr<D3D12Driver> device, int width, int height)
{
    m_SSAOTexture.reset();

    m_width = width;
    m_height = height;
    
    m_SSAOTexture = device->CreateTexture(width, height, TextureFormat::R16Norm, TextureType::Storage);
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