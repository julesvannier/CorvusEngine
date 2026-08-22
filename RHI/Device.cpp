#include "Device.h"
#include <dxgi1_6.h>

#define ENABLE_DEBUG_LAYER 1

Device::Device()
{
    HRESULT hr;

#if ENABLE_DEBUG_LAYER
    hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_debug));
    if(FAILED(hr))
    {
        LOG(Error, "Device : failed to get debug interface !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }
    m_debug->EnableDebugLayer();
#endif

    hr = CreateDXGIFactory(IID_PPV_ARGS(&m_factory));
    if(FAILED(hr))
    {
        LOG(Error, "Device : failed to create DXGI factory !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }

    // Getting the hardware adapter
    IDXGIFactory6* Factory6;
    if(SUCCEEDED(m_factory->QueryInterface(IID_PPV_ARGS(&Factory6))))
    {
        for(uint32_t AdapterIndex = 0; SUCCEEDED(Factory6->EnumAdapterByGpuPreference(AdapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&m_adapter))); ++AdapterIndex)
        {
            DXGI_ADAPTER_DESC1 Desc;
            m_adapter->GetDesc1(&Desc);

            if(Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            if(SUCCEEDED(D3D12CreateDevice((IUnknown*)m_adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device*), nullptr)))
                break;
        }
    }

    if(m_adapter == nullptr) {
        for (uint32_t AdapterIndex = 0; SUCCEEDED(m_factory->EnumAdapters1(AdapterIndex, &m_adapter)); ++AdapterIndex)
        {
            DXGI_ADAPTER_DESC1 Desc;
            m_adapter->GetDesc1(&Desc);

            if(Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                continue;

            if(SUCCEEDED(D3D12CreateDevice((IUnknown*)m_adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device*), nullptr)))
                break;
        }
    }

    hr = D3D12CreateDevice(m_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));
    if(FAILED(hr))
    {
        LOG(Error, "Device : failed to create device !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }

#if ENABLE_DEBUG_LAYER
    hr = m_device->QueryInterface(IID_PPV_ARGS(&m_debugDevice));
    if(FAILED(hr))
    {
        LOG(Error, "Device : failed to query debug device !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }
    
    ID3D12InfoQueue* InfoQueue = 0;
    m_device->QueryInterface(IID_PPV_ARGS(&InfoQueue));

    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

    D3D12_MESSAGE_SEVERITY SupressSeverities[] = {
        D3D12_MESSAGE_SEVERITY_INFO
    };

    D3D12_MESSAGE_ID SupressIDs[] = {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
        D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
    };

    D3D12_INFO_QUEUE_FILTER filter = {0};
    filter.DenyList.NumSeverities = ARRAYSIZE(SupressSeverities);
    filter.DenyList.pSeverityList = SupressSeverities;
    filter.DenyList.NumIDs = ARRAYSIZE(SupressIDs);
    filter.DenyList.pIDList = SupressIDs;

    InfoQueue->PushStorageFilter(&filter);
    InfoQueue->Release();
#endif

    DXGI_ADAPTER_DESC Desc;
    m_adapter->GetDesc(&Desc);
    std::wstring WideName = Desc.Description;
    std::string DeviceName = std::string(WideName.begin(), WideName.end());
    LOG(Debug, "Device : Using GPU : " + DeviceName);
}

Device::~Device()
{
    m_device->Release();
#if ENABLE_DEBUG_LAYER
    m_debug->Release();
    m_debugDevice->Release();
#endif
    m_factory->Release();
    m_adapter->Release();
}
