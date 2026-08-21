#pragma once
#include <Core.h>

#include "Allocator.h"
#include "DescriptorHeap.h"

enum class BufferType
{
    Vertex,
    Index,
    Constant,
    Structured,
    Copy
};

class Buffer 
{
public:
    Buffer(std::shared_ptr<Allocator> allocator, uint64_t size, uint64_t stride, BufferType type, bool readback);
    ~Buffer();

    void CreateConstantBuffer(std::shared_ptr<Device> device, std::shared_ptr<DescriptorHeap> heap);
    void CreateUnorderedAccessView(std::shared_ptr<Device> device, std::shared_ptr<DescriptorHeap> heap);

    void Map(int start, int end, void **data);
    void Unmap(int start, int end);

    void SetState(D3D12_RESOURCE_STATES state) { m_state = state; }

    GPUResource& GetResource() { return m_resource; }
    bool IsUploadHeap() const { return m_type == BufferType::Constant || m_type == BufferType::Copy || m_type == BufferType::Structured; }

private:
    friend class CommandList;

    DescriptorHandle m_descriptorHandle;
    GPUResource m_resource;
    D3D12_RESOURCE_STATES m_state;
    BufferType m_type;
    uint64_t m_size;
    std::shared_ptr<DescriptorHeap> m_heap;

    D3D12_VERTEX_BUFFER_VIEW m_VBV;
    D3D12_INDEX_BUFFER_VIEW m_IBV;
    D3D12_CONSTANT_BUFFER_VIEW_DESC m_CBVD;
    D3D12_UNORDERED_ACCESS_VIEW_DESC m_UAVD;
};