#pragma once
#include "DescriptorHeap.h"
#include "Device.h"

class Sampler
{
public:
    Sampler(std::shared_ptr<Device> device, std::shared_ptr<DescriptorHeap> samplerHeap, D3D12_TEXTURE_ADDRESS_MODE addressMode, D3D12_FILTER filter, D3D12_COMPARISON_FUNC comparisonFunc = D3D12_COMPARISON_FUNC_NEVER);
    ~Sampler();

    DescriptorHandle GetDescriptorHandle() { return m_descriptorHandle; }

private:
    DescriptorHandle m_descriptorHandle;
    std::shared_ptr<DescriptorHeap> m_heap;
};
