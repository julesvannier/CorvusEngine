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

    m_depthBuffer = renderer->CreateTexture(width, height, TextureFormat::R32Depth, TextureType::DepthTarget);
    renderer->CreateDepthView(m_depthBuffer);
    
    m_constantBuffer = renderer->CreateBuffer(256, 0, BufferType::Constant, false);
    renderer->CreateConstantBuffer(m_constantBuffer);
}

void TransparencyRenderPass::OnResize(std::shared_ptr<D3D12Renderer> renderer, int width, int height)
{
    m_depthBuffer.reset();
    m_depthBuffer = renderer->CreateTexture(width, height, TextureFormat::R32Depth, TextureType::DepthTarget);
    renderer->CreateDepthView(m_depthBuffer);
}

void TransparencyRenderPass::Pass(std::shared_ptr<D3D12Renderer> renderer, const GlobalPassData& globalPassData, const Camera& camera, const std::vector<RenderMeshData>& renderMeshesData, RenderTargetInfo renderTargetInfo)
{
    auto view = camera.GetViewMatrix();
    auto proj = camera.GetProjMatrix();

    DirectX::XMMATRIX viewProj = view * proj;
    
    SceneConstantBuffer cbuf;
    cbuf.Time = globalPassData.ElapsedTime;
    cbuf.CameraPosition = camera.GetPosition();
    cbuf.Mode = globalPassData.ViewMode;
    DirectX::XMStoreFloat4x4(&cbuf.ViewProj, DirectX::XMMatrixTranspose(viewProj));
        
    void* data;
    m_constantBuffer->Map(0, 0, &data);
    memcpy(data, &cbuf, sizeof(SceneConstantBuffer));
    m_constantBuffer->Unmap(0, 0);

    auto commandList = renderer->GetCurrentCommandList();
    auto backbuffer = renderer->GetBackBuffer();
    
    commandList->SetTopology(Topology::TriangleList);
    commandList->BindGraphicsPipeline(m_forwardTransparencyPipeline);
    commandList->BindRenderTargets({ backbuffer }, m_depthBuffer);
    commandList->ClearDepthTarget(m_depthBuffer);
    commandList->BindGraphicsConstantBuffer(m_constantBuffer, 0);
    commandList->BindGraphicsSampler(m_textureSampler, 2);

    for(const auto renderMeshData : renderMeshesData)
    {
        auto& material = renderMeshData.Material;
        
        if(material.HasAlbedo)
            commandList->BindGraphicsShaderResource(material.Albedo, 3);
    
        if(material.HasNormal)
            commandList->BindGraphicsShaderResource(material.Normal, 4);
            
        const auto primitives = renderMeshData.Primitives;
        for(const auto& primitive : primitives)
        {
            commandList->BindVertexBuffer(primitive.m_vertexBuffer);
            commandList->BindIndexBuffer(primitive.m_indicesBuffer);
            commandList->DrawIndexed(primitive.m_indexCount);
        }
    }
}