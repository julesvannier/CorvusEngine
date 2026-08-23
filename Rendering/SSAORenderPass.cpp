#include "SSAORenderPass.h"
#include "Utilities.h"
#include "Image.h"

void SSAORenderPass::Initialize(std::shared_ptr<D3D12Driver> device, int width, int height)
{
    m_sampler = device->CreateSampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR);
    
    Shader SSAOShader;
    ShaderCompiler::CompileShader("Shaders/SSAO.hlsl", ShaderType::Compute, SSAOShader);
    m_SSAOPipeline = device->CreateComputePipeline(SSAOShader);

    m_sceneConstantBuffer = device->CreateBuffer(512, 0, BufferType::Constant, false);
    device->CreateConstantBuffer(m_sceneConstantBuffer);

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

    auto commandList = device->GetCurrentCommandList();

    commandList->BindComputePipeline(m_SSAOPipeline);
    commandList->ImageBarrier(m_SSAOTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->BindComputeUnorderedAccessView(m_SSAOTexture, 0);
    commandList->BindComputeShaderResource(globalPassData.GBuffer.DepthBuffer, 1);
    commandList->BindComputeSampler(m_sampler, 2);
    commandList->BindComputeConstantBuffer(m_sceneConstantBuffer, 3);
    commandList->BindComputeShaderResource(globalPassData.GBuffer.NormalRenderTarget, 4);
    
    commandList->Dispatch((m_width + 31) / 32, (m_height + 31) / 32, 1);
    
    commandList->ImageBarrier(m_SSAOTexture, D3D12_RESOURCE_STATE_GENERIC_READ);
}