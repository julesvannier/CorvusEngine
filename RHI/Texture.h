#pragma once
#include <Core.h>

#include "Allocator.h"
#include "DescriptorHeap.h"
#include "Device.h"

enum class TextureType
{
    ShaderResource,
    RenderTarget,
    DepthTarget,
    Storage,
    Copy
};

enum class TextureFormat
{
    RGBA8 = DXGI_FORMAT_R8G8B8A8_UNORM,
    RGBA32Float = DXGI_FORMAT_R32G32B32A32_FLOAT,
    RGBA16Float = DXGI_FORMAT_R16G16B16A16_FLOAT,
    R32Depth = DXGI_FORMAT_D32_FLOAT,
    R11G11B10Float = DXGI_FORMAT_R11G11B10_FLOAT,
    RGBA8SNorm = DXGI_FORMAT_R8G8B8A8_SNORM,
    R32Float = DXGI_FORMAT_R32_FLOAT,
    RG16Float = DXGI_FORMAT_R16G16_FLOAT,
    R16Norm = DXGI_FORMAT_R16_UNORM,
    R8Norm = DXGI_FORMAT_R8_UNORM
};

class Texture 
{
public:
    Texture(std::shared_ptr<Device> device);
    Texture(std::shared_ptr<Device> device, std::shared_ptr<Allocator> allocator, uint32_t width, uint32_t height, TextureFormat format, TextureType type);
    ~Texture();

    void CreateRenderTarget(std::shared_ptr<DescriptorHeap> heap);
    void CreateDepthTarget(std::shared_ptr<DescriptorHeap> heap);
    void CreateShaderResource(std::shared_ptr<DescriptorHeap> heap);
    void CreateUnorderedAccessView(std::shared_ptr<DescriptorHeap> heap);

    void SetState(D3D12_RESOURCE_STATES state) { m_state = state; }
    D3D12_RESOURCE_STATES GetState() { return m_state; }
    GPUResource& GetResource() { return m_resource; }
    TextureFormat GetFormat() { return m_format; }
    void SetFormat(TextureFormat format) { m_format = format; }

    DescriptorHandle m_rtv;
    DescriptorHandle m_dsv;
    DescriptorHandle m_srvUav;

private:
    friend class SwapChain;
    friend class CommandList;

    std::shared_ptr<Device> m_device;
    TextureFormat m_format;
    D3D12_RESOURCE_STATES m_state;
    int m_width;
    int m_height;

    GPUResource m_resource;
    bool m_hasAlloc = false;
};