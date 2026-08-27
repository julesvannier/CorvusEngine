#include "Texture.h"

uint32_t GetFormatBytesPerPixel(TextureFormat format)
{
    switch (format)
    {
        case TextureFormat::RGBA32Float:
            return 16;
        case TextureFormat::RGBA16Float:
            return 8;
        case TextureFormat::RGBA8:
        case TextureFormat::RGBA8SRGB:
        case TextureFormat::RGBA8SNorm:
        case TextureFormat::R32Float:
        case TextureFormat::R32Depth:
        case TextureFormat::R11G11B10Float:
        case TextureFormat::RG16Float:
            return 4;
        case TextureFormat::R16Norm:
            return 2;
        case TextureFormat::R8Norm:
            return 1;
    }
    return 4;
}

uint32_t GetTextureRowPitch(TextureFormat format, uint32_t width)
{
    uint32_t unalignedPitch = width * GetFormatBytesPerPixel(format);
    return (unalignedPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
}

D3D12_RESOURCE_FLAGS GetResourceFlag(TextureType usage)
{
    switch (usage)
    {
        case TextureType::RenderTarget:
            return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        case TextureType::DepthTarget:
            return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        case TextureType::Copy:
            return D3D12_RESOURCE_FLAG_NONE;
        case TextureType::ShaderResource:
            return D3D12_RESOURCE_FLAG_NONE;
        case TextureType::Storage:
            return D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    return D3D12_RESOURCE_FLAG_NONE;
}

Texture::Texture(std::shared_ptr<Device> device) : m_device(device)
{
}

Texture::Texture(std::shared_ptr<Device> device, std::shared_ptr<Allocator> allocator, uint32_t width, uint32_t height, TextureFormat format, TextureType type, const char* name)
    : m_device(device), m_format(format), m_width(width), m_height(height)
{
    switch(type)
    {
        case TextureType::RenderTarget:
            m_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        break;
        case TextureType::DepthTarget:
            m_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        break;
        case TextureType::Storage:
            m_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        break;
        case TextureType::ShaderResource:
            m_state = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        break;
        case TextureType::Copy:
            m_state = D3D12_RESOURCE_STATE_COPY_DEST;
        break;
    }

    D3D12MA::ALLOCATION_DESC AllocationDesc = {};
    AllocationDesc.HeapType = type == TextureType::Copy ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ResourceDesc.Width = width;
    ResourceDesc.Height = height;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = (DXGI_FORMAT)format;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    ResourceDesc.Flags = GetResourceFlag(type);

    m_resource = allocator->Allocate(&AllocationDesc, &ResourceDesc, m_state);
    m_hasAlloc = true;

    if (name)
    {
        m_name = name;
        std::wstring wideName(name, name + strlen(name));
        m_resource.Resource->SetName(wideName.c_str());
    }
}

Texture::~Texture()
{
    if(m_hasAlloc)
        m_resource.Allocation->Release();
}

void Texture::CreateRenderTarget(std::shared_ptr<DescriptorHeap> heap)
{
    m_rtv = heap->Allocate();

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = (DXGI_FORMAT)m_format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    m_device->GetDevice()->CreateRenderTargetView(m_resource.Resource, &rtvDesc, m_rtv.CPU);
}

void Texture::CreateDepthTarget(std::shared_ptr<DescriptorHeap> heap)
{
    m_dsv = heap->Allocate();

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = (DXGI_FORMAT)m_format;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    m_device->GetDevice()->CreateDepthStencilView(m_resource.Resource, &dsvDesc, m_dsv.CPU);
}

void Texture::CreateShaderResource(std::shared_ptr<DescriptorHeap> heap)
{
    m_srvUav = heap->Allocate();

    D3D12_SHADER_RESOURCE_VIEW_DESC ShaderResourceView = {};
    ShaderResourceView.Format = (DXGI_FORMAT)m_format;
    ShaderResourceView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    ShaderResourceView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    ShaderResourceView.Texture2D.MipLevels = 1;

    m_device->GetDevice()->CreateShaderResourceView(m_resource.Resource, &ShaderResourceView, m_srvUav.CPU);
}

void Texture::CreateUnorderedAccessView(std::shared_ptr<DescriptorHeap> heap)
{
    m_srvUav = heap->Allocate();

    D3D12_UNORDERED_ACCESS_VIEW_DESC UnorderedAccessView = {};
    UnorderedAccessView.Format = (DXGI_FORMAT)m_format;
    UnorderedAccessView.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    m_device->GetDevice()->CreateUnorderedAccessView(m_resource.Resource, nullptr, &UnorderedAccessView, m_srvUav.CPU);
}