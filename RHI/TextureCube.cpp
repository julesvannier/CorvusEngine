#include "TextureCube.h"
#include "CommandList.h"
#include "DDSTextureLoader/DDSTextureLoader.h"

static DXGI_FORMAT ToSRGB(DXGI_FORMAT format)
{
    switch(format)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8A8_UNORM:   return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case DXGI_FORMAT_B8G8R8X8_UNORM:   return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_BC1_UNORM:        return DXGI_FORMAT_BC1_UNORM_SRGB;
        case DXGI_FORMAT_BC2_UNORM:        return DXGI_FORMAT_BC2_UNORM_SRGB;
        case DXGI_FORMAT_BC3_UNORM:        return DXGI_FORMAT_BC3_UNORM_SRGB;
        case DXGI_FORMAT_BC7_UNORM:        return DXGI_FORMAT_BC7_UNORM_SRGB;
        default:                           return format;
    }
}

TextureCube::TextureCube(std::shared_ptr<Device> device, std::shared_ptr<CommandList> cmdList, const std::wstring& filePath, Heaps& heaps)
{
    HRESULT hr = DirectX::CreateDDSTextureFromFile12(device->GetDevice(), cmdList->GetCommandList(), filePath.c_str(),m_resourceComPtr, uploadHeap);
    if(FAILED(hr))
    {
        LOG(Error, "failed to create dds texture !!!");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }

    m_width = m_resourceComPtr->GetDesc().Width;
    m_height = m_resourceComPtr->GetDesc().Height;

    auto shaderHeap = heaps.ShaderHeap;
    m_srv = shaderHeap->Allocate();
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = m_resourceComPtr->GetDesc().MipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    srvDesc.Format = ToSRGB(m_resourceComPtr->GetDesc().Format);
    device->GetDevice()->CreateShaderResourceView(m_resourceComPtr.Get(), &srvDesc, m_srv.CPU);

    m_resource.Resource = m_resourceComPtr.Get();
    m_name.resize(filePath.size());
    for (size_t i = 0; i < filePath.size(); i++)
        m_name[i] = static_cast<char>(filePath[i]);
    m_resourceComPtr->SetName(filePath.c_str());
}

TextureCube::TextureCube(std::shared_ptr<Device> device, std::shared_ptr<Allocator> allocator, uint32_t width, uint32_t height, TextureFormat format, Heaps& heaps, uint32_t mipLevels, const char* name)
{
    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 6;
    resourceDesc.MipLevels = mipLevels;
    resourceDesc.Format = DXGI_FORMAT(format);
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    m_state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    m_resource = allocator->Allocate(&allocDesc, &resourceDesc, m_state);
    m_hasAlloc = true;

    if (name)
    {
        m_name = name;
        std::wstring wideName(name, name + strlen(name));
        m_resource.Resource->SetName(wideName.c_str());
    }

    auto shaderHeap = heaps.ShaderHeap;
    m_srv = shaderHeap->Allocate();
    
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = mipLevels;
    srvDesc.Format = (DXGI_FORMAT)format;
    device->GetDevice()->CreateShaderResourceView(m_resource.Resource, &srvDesc, m_srv.CPU);

    for(uint32_t i = 0; i < mipLevels; i++)
    {
        m_uavs[i] = shaderHeap->Allocate();

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.MipSlice = i;
        uavDesc.Texture2DArray.ArraySize = 6;
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Format = DXGI_FORMAT(format);

        device->GetDevice()->CreateUnorderedAccessView(m_resource.Resource, nullptr, &uavDesc, m_uavs[i].CPU);
    }
}

TextureCube::~TextureCube()
{
    if(m_hasAlloc)
        m_resource.Allocation->Release();
}
