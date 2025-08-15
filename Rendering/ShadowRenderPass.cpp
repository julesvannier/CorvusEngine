#include "ShadowRenderPass.h"

void ShadowRenderPass::Initialize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
    GraphicsPipelineSpecs shadowSpecs;
    shadowSpecs.FormatCount = 0;
    shadowSpecs.DepthEnabled = true;
    shadowSpecs.Depth = DepthOperation::Less;
    shadowSpecs.DepthFormat = TextureFormat::R32Depth;
    shadowSpecs.Cull = CullMode::Front;
    shadowSpecs.Fill = FillMode::Solid;
    shadowSpecs.BlendOperation = BlendOperation::None;
    ShaderCompiler::CompileShader("Shaders/ShadowMapVertex.hlsl", ShaderType::Vertex, shadowSpecs.ShadersBytecodes[ShaderType::Vertex]);
    ShaderCompiler::CompileShader("Shaders/ShadowMapPixel.hlsl", ShaderType::Pixel, shadowSpecs.ShadersBytecodes[ShaderType::Pixel]);

    m_shadowPipeline = renderer->CreateGraphicsPipeline(shadowSpecs);

    m_constantBuffer = renderer->CreateBuffer(256, 0, BufferType::Constant, false);
    renderer->CreateConstantBuffer(m_constantBuffer);

    OnResize(renderer, width, height);
}

void ShadowRenderPass::Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTarget)
{
    float sceneBoundsRadius = 15.0f;
    
    DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&globalPassData.DirectionalInfo.Direction);
    DirectX::XMVECTOR lightPos = DirectX::XMVectorScale(lightDir, -2.0f * sceneBoundsRadius);
    DirectX::XMFLOAT3 worldOrigin = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&worldOrigin);
    DirectX::XMVECTOR lightUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    DirectX::XMMATRIX lightView = DirectX::XMMatrixLookAtLH(lightPos, targetPos, lightUp);

    // Transform bounding sphere to light space.
    DirectX::XMFLOAT3 sphereCenterLS;
    DirectX::XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

    // Ortho frustum in light space encloses scene.
    float l = sphereCenterLS.x - sceneBoundsRadius;
    float b = sphereCenterLS.y - sceneBoundsRadius;
    float n = sphereCenterLS.z - sceneBoundsRadius;
    float r = sphereCenterLS.x + sceneBoundsRadius;
    float t = sphereCenterLS.y + sceneBoundsRadius;
    float f = sphereCenterLS.z + sceneBoundsRadius;

    DirectX::XMMATRIX lightProj = DirectX::XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

    // Transform NDC space [-1,+1]^2 to texture space [0,1]^2
    DirectX::XMMATRIX T(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f);

    DirectX::XMMATRIX S = lightView * lightProj * T;
    DirectX::XMStoreFloat4x4(&m_shadowMap.ShadowTransform, S);

    DirectX::XMMATRIX viewProj = lightView * lightProj;

    ShadowMapConstantBuffer cbuf;
    DirectX::XMStoreFloat4x4(&cbuf.ViewProj, viewProj);

    void* data;
    m_constantBuffer->Map(0, 0, &data);
    memcpy(data, &cbuf, sizeof(ShadowMapConstantBuffer));
    m_constantBuffer->Unmap(0, 0);
    
    auto commandList = renderer->GetCurrentCommandList();

    commandList->ImageBarrier(m_shadowMap.DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ClearDepthTarget(m_shadowMap.DepthBuffer);
    commandList->SetViewport(0, 0, m_shadowMapWidth, m_shadowMapHeight);

    commandList->BindDepthTarget(m_shadowMap.DepthBuffer);

    commandList->SetTopology(Topology::TriangleList);
    commandList->BindGraphicsPipeline(m_shadowPipeline);
    commandList->BindGraphicsConstantBuffer(m_constantBuffer, 0);

    for(const auto renderMeshData : renderMeshesData)
    {
        auto& material = renderMeshData.Material;

        if (!material.CastShadows)
            continue;
        
        std::vector<InstanceData> instancesData;
        for(auto instanceTransform : renderMeshData.InstancesTransforms)
        {
            InstanceData instanceData;
            instanceData.WorldMat = instanceTransform;
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

    commandList->ImageBarrier(m_shadowMap.DepthBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
}

void ShadowRenderPass::OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
    m_shadowMapWidth = width;
    m_shadowMapHeight = height;
    
    m_shadowMap.DepthBuffer.reset();

    m_shadowMap.DepthBuffer = renderer->CreateTexture(width, height, TextureFormat::R32Depth, TextureType::DepthTarget);
    renderer->CreateDepthView(m_shadowMap.DepthBuffer);
    m_shadowMap.DepthBuffer->SetFormat(TextureFormat::R32Float);
    renderer->CreateShaderResourceView(m_shadowMap.DepthBuffer);
    m_shadowMap.DepthBuffer->SetFormat(TextureFormat::R32Depth);
}
