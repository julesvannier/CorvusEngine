#include "TransparencyRenderPass.h"

void TransparencyRenderPass::Initialize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
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
    ShaderCompiler::CompileShader("Shaders/SimpleVertex.hlsl", ShaderType::Vertex, specs.ShadersBytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("Shaders/TransparentPixel.hlsl", ShaderType::Pixel, specs.ShadersBytecodes[ShaderType::Pixel]);

    m_forwardTransparencyPipeline = renderer->CreateGraphicsPipeline(specs);

    m_sceneConstantBuffer = renderer->CreateBuffer(256, 0, BufferType::Constant, false);
    renderer->CreateConstantBuffer(m_sceneConstantBuffer);

    m_opacityValuesBuffer = renderer->CreateBuffer(sizeof(float) * MAX_TRANSPARENT_OBJECTS, sizeof(float), BufferType::Structured, false);
}

void TransparencyRenderPass::OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
}

void TransparencyRenderPass::Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTargetInfo)
{
    // Prepass to list all Opacity values
    bool foundTransparentMesh = false;
    std::vector<float> opacityData;
    for (const auto renderMeshData : renderMeshesData)
    {
        auto& material = renderMeshData.Material;

        if (!material.IsTransparent)
            continue;

        opacityData.emplace_back(material.Opacity);
        foundTransparentMesh = true;
    }

    if (!foundTransparentMesh)
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
    if (data)
    {
        memcpy(data, &cbuf, sizeof(SceneConstantBuffer));
        m_sceneConstantBuffer->Unmap(0, 0);
    }

    auto commandList = renderer->GetCurrentCommandList();
    
    commandList->SetTopology(Topology::TriangleList);
    commandList->BindGraphicsPipeline(m_forwardTransparencyPipeline);
    commandList->ImageBarrier(renderTargetInfo.RenderTexture, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ImageBarrier(renderTargetInfo.DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->BindRenderTargets({ renderTargetInfo.RenderTexture }, renderTargetInfo.DepthBuffer);
    commandList->BindGraphicsConstantBuffer(m_sceneConstantBuffer, 0);
    commandList->BindGraphicsSampler(m_textureSampler, 3);
    commandList->BindGraphicsShaderResource(globalPassData.EnviroMaps.DiffuseIrradianceMap, 7);
    commandList->BindGraphicsShaderResource(globalPassData.EnviroMaps.PrefilterEnvMap, 8);
    
    void* opacityDt;
    m_opacityValuesBuffer->Map(0, 0, &opacityDt);
    if (opacityDt)
    {
        memcpy(opacityDt, opacityData.data(), sizeof(InstanceData) * renderMeshesData.size());
        m_opacityValuesBuffer->Unmap(0, 0);
    }
    
    commandList->SetGraphicsShaderResource(m_opacityValuesBuffer, 2);

    for(const auto renderMeshData : renderMeshesData)
    {
        auto& material = renderMeshData.Material;

        if (!material.IsTransparent)
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
        if (dt)
        {
            memcpy(dt, instancesData.data(), sizeof(InstanceData) * renderMeshData.InstancesTransforms.size());
            renderMeshData.InstancesDataBuffer->Unmap(0, 0);
        }

        commandList->SetGraphicsShaderResource(renderMeshData.InstancesDataBuffer, 1);

        if(material.HasAlbedo)
            commandList->BindGraphicsShaderResource(material.Albedo, 4);
        
        if(material.HasNormal)
            commandList->BindGraphicsShaderResource(material.Normal, 5);
        
        if(material.HasMetallicRoughness)
            commandList->BindGraphicsShaderResource(material.MetallicRoughness, 6);
            
        const auto primitives = renderMeshData.Primitives;
        for(const auto& primitive : primitives)
        {
            commandList->BindVertexBuffer(primitive.m_vertexBuffer);
            commandList->BindIndexBuffer(primitive.m_indicesBuffer);
            commandList->DrawIndexed(primitive.m_indexCount, renderMeshData.InstancesTransforms.size());
        }
    }

    commandList->ImageBarrier(renderTargetInfo.RenderTexture, D3D12_RESOURCE_STATE_GENERIC_READ);
    commandList->ImageBarrier(renderTargetInfo.DepthBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
}