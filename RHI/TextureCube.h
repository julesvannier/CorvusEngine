#pragma once
#include <Core.h>
#include <wrl/client.h>
#include "Allocator.h"
#include "DescriptorHeap.h"
#include "Device.h"
#include "Texture.h"

class CommandList;

class TextureCube
{
public:
    TextureCube(std::shared_ptr<Device> device, std::shared_ptr<CommandList> cmdList, const std::wstring& filePath, Heaps& heaps);
    TextureCube(std::shared_ptr<Device> device, std::shared_ptr<Allocator> allocator, uint32_t width, uint32_t height, TextureFormat format, Heaps& heaps, uint32_t mipLevels = 5, const char* name = nullptr);
    ~TextureCube();
    
    DescriptorHandle& GetSrvHandle() { return m_srv; }
    GPUResource& GetResource() { return m_resource; }
    D3D12_RESOURCE_STATES GetState() { return m_state; }
    void SetState(D3D12_RESOURCE_STATES state) { m_state = state; }
    const std::string& GetName() { return m_name; }

private:
    friend class CommandList;
    DescriptorHandle m_srv;
    DescriptorHandle m_uavs[6];

    std::string m_name;
    uint32_t m_width, m_height;
    D3D12_RESOURCE_STATES m_state;
    GPUResource m_resource;
    bool m_hasAlloc = false;
    
    Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resourceComPtr;
};
