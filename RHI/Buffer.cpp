#include "Buffer.h"

Buffer::Buffer(std::shared_ptr<Allocator> allocator, uint64_t size, uint64_t stride, BufferType type, bool readback) : m_size(size)
{
    D3D12MA::ALLOCATION_DESC AllocationDesc = {};
    AllocationDesc.HeapType = readback == true ? D3D12_HEAP_TYPE_READBACK : ((type == BufferType::Constant || type == BufferType::Copy || type == BufferType::Structured ) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    ResourceDesc.Width = size;
    ResourceDesc.Height = 1;
    ResourceDesc.DepthOrArraySize = 1;
    ResourceDesc.MipLevels = 1;
    ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.SampleDesc.Count = 1;
    ResourceDesc.SampleDesc.Quality = 0;
    ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    m_state = type == BufferType::Constant || type == BufferType::Structured ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;

    m_resource = allocator->Allocate(&AllocationDesc, &ResourceDesc, m_state);

    switch (type) {
        case BufferType::Vertex: {
            m_VBV.BufferLocation = m_resource.Resource->GetGPUVirtualAddress();
            m_VBV.SizeInBytes = size;
            m_VBV.StrideInBytes = stride;
            break;
        }
        case BufferType::Index: {
            m_IBV.BufferLocation = m_resource.Resource->GetGPUVirtualAddress();
            m_IBV.SizeInBytes = size;
            m_IBV.Format = DXGI_FORMAT_R32_UINT;
            break;
        }
        default: {
            break;
        }
    }

    m_descriptorHandle = DescriptorHandle();
}

Buffer::~Buffer()
{
    if(m_descriptorHandle.IsValid())
        m_heap->Free(m_descriptorHandle);

    m_resource.Allocation->Release();
}

void Buffer::CreateConstantBuffer(std::shared_ptr<Device> device, std::shared_ptr<DescriptorHeap> heap)
{
    m_heap = heap;

    m_CBVD.BufferLocation = m_resource.Resource->GetGPUVirtualAddress();
    m_CBVD.SizeInBytes = m_size;
    if(m_descriptorHandle.IsValid() == false)
        m_descriptorHandle = heap->Allocate();

    device->GetDevice()->CreateConstantBufferView(&m_CBVD, m_descriptorHandle.CPU);
}

void Buffer::CreateUnorderedAccessView(std::shared_ptr<Device> device, std::shared_ptr<DescriptorHeap> heap)
{
    m_heap = heap;

    m_UAVD.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    m_UAVD.Format = DXGI_FORMAT_UNKNOWN;
    if(m_descriptorHandle.IsValid() == false)
        m_descriptorHandle = heap->Allocate();

    device->GetDevice()->CreateUnorderedAccessView(m_resource.Resource, nullptr, &m_UAVD, m_descriptorHandle.CPU);
}

void Buffer::Map(int start, int end, void **data)
{
    *data = nullptr;

    D3D12_RANGE range;
    range.Begin = start;
    range.End = end;

    HRESULT hr = 0;
    if(range.End > range.Begin)
    {
        hr = m_resource.Resource->Map(0, &range, data);
    }
    else
    {
        hr = m_resource.Resource->Map(0, nullptr, data);
    }

    if(FAILED(hr))
    {
        LOG(Error, "Buffer : failed to map buffer !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }
}

void Buffer::Unmap(int start, int end)
{
    D3D12_RANGE range;
    range.Begin = start;
    range.End = end;

    if (range.End > range.Begin)
    {
        m_resource.Resource->Unmap(0, &range);
    }
    else
    {
        m_resource.Resource->Unmap(0, nullptr);
    }
}