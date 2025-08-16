#pragma once
#include <Core.h>

#include "Buffer.h"
#include "ComputePipeline.h"
#include "Device.h"
#include "GraphicsPipeline.h"
#include "Sampler.h"
#include "Texture.h"
#include "TextureCube.h"

enum class Topology
{
    LineList = D3D_PRIMITIVE_TOPOLOGY_LINELIST,
    LineStrip = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
    PointList = D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
    TriangleList = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
    TriangleStrip = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
    QuadPatch = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST  
};

class CommandList
{
public:
    CommandList(std::shared_ptr<Device> device, const Heaps& heaps, D3D12_COMMAND_LIST_TYPE commandQueueType);
    ~CommandList();

    void Begin();
    void End();

    void ImageBarrier(std::shared_ptr<Texture> texture, D3D12_RESOURCE_STATES state);
    void ImageBarrier(std::shared_ptr<TextureCube> texture, D3D12_RESOURCE_STATES state);
    void ImageBarrier(std::unordered_map<std::shared_ptr<Texture>, D3D12_RESOURCE_STATES> texturesStates);
    void BindRenderTargets(const std::vector<std::shared_ptr<Texture>>& renderTargets, std::shared_ptr<Texture> depthTarget);
    void BindDepthTarget(std::shared_ptr<Texture> depthTarget);
    void ClearRenderTarget(std::shared_ptr<Texture> renderTarget, float r, float g, float b, float a);
    void ClearDepthTarget(std::shared_ptr<Texture> depthTarget);
    void SetViewport(float x, float y, float width, float height);
    void SetTopology(Topology topology);
    void BindVertexBuffer(std::shared_ptr<Buffer> buffer);
    void BindIndexBuffer(std::shared_ptr<Buffer> buffer);
    void BindGraphicsConstantBuffer(std::shared_ptr<Buffer> buffer, int idx);
    void BindComputeConstantBuffer(std::shared_ptr<Buffer> buffer, int idx);
    void BindGraphicsPipeline(std::shared_ptr<GraphicsPipeline> pipeline);
    void BindComputePipeline(std::shared_ptr<ComputePipeline> pipeline);
    void BindGraphicsShaderResource(std::shared_ptr<Texture> texture, int idx);
    void BindComputeUnorderedAccessView(std::shared_ptr<Texture> texture, int idx);
    void BindComputeUnorderedAccessView(std::shared_ptr<TextureCube> texture, int idx, int mip);
    void BindGraphicsShaderResource(std::shared_ptr<TextureCube> texture, int idx);
    void BindComputeShaderResource(std::shared_ptr<TextureCube> texture, int idx);
    void BindComputeShaderResource(std::shared_ptr<Texture> texture, int idx);
    void BindGraphicsSampler(std::shared_ptr<Sampler> sampler, int idx);
    void BindComputeSampler(std::shared_ptr<Sampler> sampler, int idx);
    void SetGraphicsShaderResource(std::shared_ptr<Buffer> buffer, int idx);
    void SetComputeShaderResource(std::shared_ptr<Buffer> buffer, int idx);
    void Draw(int vertexCount, int instanceCount = 1);
    void DrawIndexed(int indexCount, int instanceCount = 1);
    void Dispatch(int x, int y, int z);
    void CopyTextureToTexture(std::shared_ptr<Texture> dst, std::shared_ptr<Texture> src);
    void CopyBufferToBuffer(std::shared_ptr<Buffer> dst, std::shared_ptr<Buffer> src);
    void CopyBufferToTexture(std::shared_ptr<Texture> dst, std::shared_ptr<Buffer> src);

    ID3D12GraphicsCommandList* GetCommandList() { return m_commandList; }

private:
    ID3D12CommandAllocator* m_commandAllocator;
    ID3D12GraphicsCommandList* m_commandList;
    D3D12_COMMAND_LIST_TYPE m_type;
    Heaps m_heaps;
};