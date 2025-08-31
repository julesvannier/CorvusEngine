#include "WaterRenderPass.h"
#include "Image.h"

void WaterRenderPass::Initialize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
    m_textureSampler = renderer->CreateSampler(D3D12_TEXTURE_ADDRESS_MODE_WRAP,  D3D12_FILTER_MIN_MAG_MIP_LINEAR);
    
    GraphicsPipelineSpecs specs;
    specs.FormatCount = 1;
    specs.Formats[0] = TextureFormat::RGBA8;
    specs.BlendOperation = BlendOperation::Transparency;
    specs.DepthEnabled = true;
    specs.Depth = DepthOperation::Less;
    specs.DepthFormat = TextureFormat::R32Depth;
    specs.Cull = CullMode::Back;
    specs.Fill = FillMode::Solid;
    specs.Topology = Patch;
    specs.TessellationEnable = true;
    ShaderCompiler::CompileShader("Shaders/WaterVertex.hlsl", ShaderType::Vertex, specs.ShadersBytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("Shaders/WaterHullShader.hlsl", ShaderType::Hull, specs.ShadersBytecodes[ShaderType::Hull]);
    ShaderCompiler::CompileShader("Shaders/WaterDomainShader.hlsl", ShaderType::Domain, specs.ShadersBytecodes[ShaderType::Domain]);
    ShaderCompiler::CompileShader("Shaders/WaterPixel.hlsl", ShaderType::Pixel, specs.ShadersBytecodes[ShaderType::Pixel]);

    m_waterTesselationPipeline = renderer->CreateGraphicsPipeline(specs);
    
    m_sceneConstantBuffer = renderer->CreateBuffer(256, 0, BufferType::Constant, false);
    renderer->CreateConstantBuffer(m_sceneConstantBuffer);

    m_waterConstantBuffer = renderer->CreateBuffer(256, 0, BufferType::Constant, false);
    renderer->CreateConstantBuffer(m_waterConstantBuffer);

    Uploader uploader = renderer->CreateUploader();
    
    Image normalImg;
    normalImg.LoadImageFromFile("Assets/waterNM1.png");
    m_normalMap = renderer->CreateTexture(normalImg.Width, normalImg.Height, TextureFormat::RGBA8, TextureType::ShaderResource);
    renderer->CreateShaderResourceView(m_normalMap);
    uploader.CopyHostToDeviceTexture(normalImg, m_normalMap);

    Image normalImg2;
    normalImg2.LoadImageFromFile("Assets/waterNM2.png");
    m_normalMap2 = renderer->CreateTexture(normalImg2.Width, normalImg2.Height, TextureFormat::RGBA8, TextureType::ShaderResource);
    renderer->CreateShaderResourceView(m_normalMap2);
    uploader.CopyHostToDeviceTexture(normalImg2, m_normalMap2);
    
    renderer->FlushUploader(uploader);
}

void WaterRenderPass::OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
}

void WaterRenderPass::Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTargetInfo)
{
    bool foundWaterMesh = false;
    for (const auto renderMeshData : renderMeshesData)
    {
        auto& material = renderMeshData.Material;

        if (!material.IsWater)
            continue;

        foundWaterMesh = true;
    }

    if (!foundWaterMesh)
        return;
    
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

    WaterConstantBuffer waterCb;
    waterCb = globalPassData.WaterParams;
    DirectX::XMStoreFloat4x4(&waterCb.View, view);
    DirectX::XMStoreFloat4x4(&waterCb.Proj, proj);

    void* data2;
    m_waterConstantBuffer->Map(0, 0, &data2);
    memcpy(data2, &waterCb, sizeof(WaterConstantBuffer));
    m_waterConstantBuffer->Unmap(0, 0);

    auto commandList = renderer->GetCurrentCommandList();
    
    commandList->SetTopology(Topology::QuadPatch);
    commandList->BindGraphicsPipeline(m_waterTesselationPipeline);
    commandList->ImageBarrier(renderTargetInfo.RenderTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->BindRenderTargets({ renderTargetInfo.RenderTexture }, renderTargetInfo.DepthBuffer /* TODO is it an issue ? I think it's fine */);
    commandList->BindGraphicsConstantBuffer(m_sceneConstantBuffer, 0);
    commandList->BindGraphicsConstantBuffer(m_waterConstantBuffer, 2);
    commandList->BindGraphicsSampler(m_textureSampler, 3);
    commandList->BindGraphicsShaderResource(m_normalMap, 4);
    commandList->BindGraphicsShaderResource(m_normalMap2, 5);
    commandList->BindGraphicsShaderResource(globalPassData.GBuffer.DepthBuffer, 6);
    commandList->BindGraphicsShaderResource(globalPassData.GBuffer.AlbedoRenderTarget, 7);
    commandList->BindGraphicsShaderResource(globalPassData.GBuffer.NormalRenderTarget, 8);
    // commandList->BindGraphicsShaderResource(globalPassData.EnviroMaps.SkyBox, ?);
    
    for(const auto renderMeshData : renderMeshesData)
    {
        auto& material = renderMeshData.Material;

        if (!material.IsWater)
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

        const auto primitives = renderMeshData.Primitives;
        for(const auto& primitive : primitives)
        {
            commandList->BindVertexBuffer(primitive.m_vertexBuffer);
            commandList->BindIndexBuffer(primitive.m_indicesBuffer);
            commandList->DrawIndexed(primitive.m_indexCount, renderMeshData.InstancesTransforms.size());
        }
    }

    commandList->ImageBarrier(renderTargetInfo.RenderTexture, D3D12_RESOURCE_STATE_GENERIC_READ);
}