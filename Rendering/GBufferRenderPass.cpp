#include "GBufferRenderPass.h"

void GBufferRenderPass::Initialize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
    m_textureSampler = renderer->CreateSampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP,  D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    GraphicsPipelineSpecs geomSpecs;
    geomSpecs.FormatCount = 4;
    geomSpecs.Formats[0] = TextureFormat::R11G11B10Float;
    geomSpecs.Formats[1] = TextureFormat::RGBA8SNorm;
    geomSpecs.Formats[2] = TextureFormat::R11G11B10Float;
    geomSpecs.Formats[3] = TextureFormat::R11G11B10Float;
    geomSpecs.DepthEnabled = true;
    geomSpecs.Depth = DepthOperation::Less;
    geomSpecs.DepthFormat = TextureFormat::R32Depth;
    geomSpecs.Cull = CullMode::Back;
    geomSpecs.Fill = FillMode::Solid;
    geomSpecs.BlendOperation = BlendOperation::None;
    ShaderCompiler::CompileShader("Shaders/SimpleVertex.hlsl", ShaderType::Vertex, geomSpecs.ShadersBytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("Shaders/DeferredGBufferPixel.hlsl", ShaderType::Pixel, geomSpecs.ShadersBytecodes[ShaderType::Pixel]);

    m_deferredGeometryPipeline = renderer->CreateGraphicsPipeline(geomSpecs);

    m_sceneConstantBuffer = renderer->CreateBuffer(256, 0, BufferType::Constant, false);
    renderer->CreateConstantBuffer(m_sceneConstantBuffer);

    OnResize(renderer, width, height);
}

void GBufferRenderPass::OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
    m_GBuffer.AlbedoRenderTarget.reset();
    m_GBuffer.NormalRenderTarget.reset();
    m_GBuffer.MetallicRoughnessRenderTarget.reset();
    m_GBuffer.DepthBuffer.reset();
    
    m_GBuffer.DepthBuffer = renderer->CreateTexture(width, height, TextureFormat::R32Depth, TextureType::DepthTarget);
    renderer->CreateDepthView(m_GBuffer.DepthBuffer);
    m_GBuffer.DepthBuffer->SetFormat(TextureFormat::R32Float);
    renderer->CreateShaderResourceView(m_GBuffer.DepthBuffer);
    m_GBuffer.DepthBuffer->SetFormat(TextureFormat::R32Depth);
    
    m_GBuffer.AlbedoRenderTarget = renderer->CreateTexture(width, height, TextureFormat::R11G11B10Float, TextureType::RenderTarget);
    renderer->CreateRenderTargetView(m_GBuffer.AlbedoRenderTarget);
    renderer->CreateShaderResourceView(m_GBuffer.AlbedoRenderTarget);

    m_GBuffer.NormalRenderTarget = renderer->CreateTexture(width, height, TextureFormat::RGBA8SNorm, TextureType::RenderTarget);
    renderer->CreateRenderTargetView(m_GBuffer.NormalRenderTarget);
    renderer->CreateShaderResourceView(m_GBuffer.NormalRenderTarget);

    m_GBuffer.MetallicRoughnessRenderTarget = renderer->CreateTexture(width, height, TextureFormat::R11G11B10Float, TextureType::RenderTarget);
    renderer->CreateRenderTargetView(m_GBuffer.MetallicRoughnessRenderTarget);
    renderer->CreateShaderResourceView(m_GBuffer.MetallicRoughnessRenderTarget);
}

void GBufferRenderPass::Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget)
{
    auto view = camera.GetViewMatrix();
    auto proj = camera.GetProjMatrix();
    auto invViewProj = camera.GetInvViewProjMatrix();

    DirectX::XMMATRIX viewProj = view * proj;
    
    SceneConstantBuffer cbuf;
    cbuf.Time = globalPassData.ElapsedTime;
    cbuf.CameraPosition = camera.GetPosition();
    cbuf.Mode = globalPassData.ViewMode;
    cbuf.DirLightDirection = globalPassData.DirectionalInfo.Direction;
    cbuf.DirLightIntensity = globalPassData.DirectionalInfo.Intensity;
    cbuf.ScreenDimensions[0] = globalPassData.ViewportSizeX;
    cbuf.ScreenDimensions[1] = globalPassData.ViewportSizeY;
    DirectX::XMStoreFloat4x4(&cbuf.ViewProj, viewProj);
    DirectX::XMStoreFloat4x4(&cbuf.InvViewProj, invViewProj);
    cbuf.ShadowTransform = globalPassData.ShadowMap.ShadowTransform;
    cbuf.ShadowEnabled = globalPassData.EnableShadows;
        
    void* data;
    m_sceneConstantBuffer->Map(0, 0, &data);
    memcpy(data, &cbuf, sizeof(SceneConstantBuffer));
    m_sceneConstantBuffer->Unmap(0, 0);
    
    auto commandList = renderer->GetCurrentCommandList();

    commandList->SetViewport(0, 0, globalPassData.ViewportSizeX, globalPassData.ViewportSizeY);

    std::unordered_map<std::shared_ptr<Texture>, D3D12_RESOURCE_STATES> renderTargetsBatchedBarriers;
    renderTargetsBatchedBarriers.emplace(m_GBuffer.AlbedoRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderTargetsBatchedBarriers.emplace(m_GBuffer.NormalRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderTargetsBatchedBarriers.emplace(m_GBuffer.MetallicRoughnessRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
    renderTargetsBatchedBarriers.emplace(m_GBuffer.DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ImageBarrier(renderTargetsBatchedBarriers);

    commandList->ClearRenderTarget(m_GBuffer.AlbedoRenderTarget, 0.0f, 0.0f, 0.0f, 1.0f);
    commandList->ClearRenderTarget(m_GBuffer.NormalRenderTarget, 0.0f, 0.0f, 0.0f, 1.0f);
    commandList->ClearRenderTarget(m_GBuffer.MetallicRoughnessRenderTarget, 0.0f, 0.0f, 0.0f, 1.0f);
    commandList->ClearDepthTarget(m_GBuffer.DepthBuffer);

    commandList->BindRenderTargets({  m_GBuffer.AlbedoRenderTarget,
                                        m_GBuffer.NormalRenderTarget,
                                        m_GBuffer.MetallicRoughnessRenderTarget },
                                        m_GBuffer.DepthBuffer);

    commandList->SetTopology(Topology::TriangleList);
    commandList->BindGraphicsPipeline(m_deferredGeometryPipeline);
    commandList->BindGraphicsConstantBuffer(m_sceneConstantBuffer, 0);
    commandList->BindGraphicsSampler(m_textureSampler, 2);

    for(const auto renderMeshData : renderMeshesData)
    {
        auto& material = renderMeshData.Material;

        if (material.IsTransparent || material.IsWater)
            continue;

        std::vector<InstanceData> instancesData;
        for(auto instanceTransform : renderMeshData.InstancesTransforms)
        {
            InstanceData instanceData;
            instanceData.WorldMat = instanceTransform;
            instanceData.HasAlbedo = material.HasAlbedo;
            instanceData.HasNormalMap = material.HasNormal;
            instanceData.HasMetallicRoughness = material.HasMetallicRoughness;
            instancesData.emplace_back(instanceData);
        }

        void* dt;
        renderMeshData.InstancesDataBuffer->Map(0, 0, &dt);
        memcpy(dt, instancesData.data(), sizeof(InstanceData) * renderMeshData.InstancesTransforms.size());
        renderMeshData.InstancesDataBuffer->Unmap(0, 0);

        commandList->SetGraphicsShaderResource(renderMeshData.InstancesDataBuffer, 1);

        if(material.HasAlbedo)
            commandList->BindGraphicsShaderResource(material.Albedo, 3);

        if(material.HasNormal)
            commandList->BindGraphicsShaderResource(material.Normal, 4);

        if(material.HasMetallicRoughness)
            commandList->BindGraphicsShaderResource(material.MetallicRoughness, 5);
            
        const auto primitives = renderMeshData.Primitives;
        for(const auto& primitive : primitives)
        {
            commandList->BindVertexBuffer(primitive.m_vertexBuffer);
            commandList->BindIndexBuffer(primitive.m_indicesBuffer);
            commandList->DrawIndexed(primitive.m_indexCount, renderMeshData.InstancesTransforms.size());
        }
    }

    std::unordered_map<std::shared_ptr<Texture>, D3D12_RESOURCE_STATES> readBatchedBarriers;
    readBatchedBarriers.emplace(m_GBuffer.AlbedoRenderTarget, D3D12_RESOURCE_STATE_GENERIC_READ);
    readBatchedBarriers.emplace(m_GBuffer.NormalRenderTarget, D3D12_RESOURCE_STATE_GENERIC_READ);
    readBatchedBarriers.emplace(m_GBuffer.MetallicRoughnessRenderTarget, D3D12_RESOURCE_STATE_GENERIC_READ);
    readBatchedBarriers.emplace(m_GBuffer.DepthBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
    commandList->ImageBarrier(readBatchedBarriers);
}