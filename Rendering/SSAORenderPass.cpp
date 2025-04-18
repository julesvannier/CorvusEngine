#include "SSAORenderPass.h"
#include "Utilities.h"

void SSAORenderPass::Initialize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
    m_sampler = renderer->CreateSampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR);
    
    Shader SSAOShader;
    ShaderCompiler::CompileShader("Shaders/SSAO.hlsl", ShaderType::Compute, SSAOShader);
    m_SSAOPipeline = renderer->CreateComputePipeline(SSAOShader);

    m_constantBuffer = renderer->CreateBuffer(256, 0, BufferType::Constant, false);
    renderer->CreateConstantBuffer(m_constantBuffer);

    constexpr int kernelSize = 16;
    for (int i = 0; i < kernelSize; i++)
    {
        m_sampleKernel[i] = DirectX::XMFLOAT3 {
            Utilities::RandomFloatRange(-1.0f, 1.0f),
            Utilities::RandomFloatRange(-1.0f, 1.0f),
            Utilities::RandomFloatRange(0.0f, 1.0f),
        };

        float scale = float(i) / float(kernelSize);
        scale = Utilities::Lerp(0.1f, 1.0f, scale * scale);
        DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&m_sampleKernel[i]);
        DirectX::XMStoreFloat3(&m_sampleKernel[i], DirectX::XMVectorScale(v, scale));

        /*
        LOG(Debug, "SSAO Sample Kernel Vector: x = " + std::to_string(m_sampleKernel[i].x) +
                    ", y = " + std::to_string(m_sampleKernel[i].y) +
                    ", z = " + std::to_string(m_sampleKernel[i].z));
        */
    }

    for (int i = 0; i < 64; i++)
    {
        m_noiseTexture[i] = DirectX::XMFLOAT3 {
            Utilities::RandomFloatRange(-1.0f, 1.0f),
            Utilities::RandomFloatRange(-1.0f, 1.0f),
            0.0f,
        };

        DirectX::XMVECTOR v = DirectX::XMLoadFloat3(&m_noiseTexture[i]);
        DirectX::XMStoreFloat3(&m_noiseTexture[i], DirectX::XMVector3Normalize(v));

        /*
        LOG(Debug, "SSAO Noise Texture Vector: x = " + std::to_string(m_noiseTexture[i].x) +
                    ", y = " + std::to_string(m_noiseTexture[i].y) +
                    ", z = " + std::to_string(m_noiseTexture[i].z));
        */
    }

    OnResize(renderer, width, height);
}

void SSAORenderPass::OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
    m_SSAOTexture.reset();
    
    m_SSAOTexture = renderer->CreateTexture(width, height, TextureFormat::R16Norm, TextureType::Storage);
    renderer->CreateUnorderedAccessView(m_SSAOTexture);
    renderer->CreateShaderResourceView(m_SSAOTexture);
}

void SSAORenderPass::Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget)
{
    SSAOConstantBuffer constantBuffer;
    constantBuffer.Value = 0.7f;

    void* data;
    m_constantBuffer->Map(0, 0, &data);
    memcpy(data, &constantBuffer, sizeof(SSAOConstantBuffer));
    m_constantBuffer->Unmap(0, 0);

    auto commandList = renderer->GetCurrentCommandList();

    commandList->BindComputePipeline(m_SSAOPipeline);
    commandList->ImageBarrier(m_SSAOTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->BindComputeUnorderedAccessView(m_SSAOTexture, 0);
    // commandList->BindComputeConstantBuffer(m_constantBuffer, 1);
    commandList->BindComputeShaderResource(globalPassData.GBuffer.DepthBuffer, 1);
    commandList->BindComputeSampler(m_sampler, 2);
    
    commandList->Dispatch(m_width / 32, m_height / 32, 6);
    
    commandList->ImageBarrier(m_SSAOTexture, D3D12_RESOURCE_STATE_GENERIC_READ);
}