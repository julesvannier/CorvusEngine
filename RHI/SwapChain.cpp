#include "SwapChain.h"

SwapChain::SwapChain(std::shared_ptr<Device> device, std::shared_ptr<CommandQueue> directQueue, std::shared_ptr<DescriptorHeap> rtvHeap, HWND window)
    : m_device(device), m_rtvHeap(rtvHeap), m_window(window)
{
    RECT ClientRect;
    GetClientRect(m_window, &ClientRect);
    m_width = ClientRect.right - ClientRect.left;
    m_height = ClientRect.bottom - ClientRect.top;

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = FRAMES_IN_FLIGHT;
    desc.Scaling = DXGI_SCALING_NONE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Width = m_width;
    desc.Height = m_height;

    IDXGISwapChain1* temp;
    HRESULT hr = device->GetFactory()->CreateSwapChainForHwnd(directQueue->GetCommandQueue(), m_window, &desc, nullptr, nullptr, &temp);
    if(FAILED(hr))
    {
        LOG(Error, "SwapChain : failed to create swap chain !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }
    temp->QueryInterface(IID_PPV_ARGS(&m_swapChain));
    temp->Release();

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i]));
        if(FAILED(hr))
        {
            LOG(Error, "SwapChain : failed to get backbuffer !");
            std::string errorMsg = std::system_category().message(hr);
            LOG(Error, errorMsg);
        }

        m_descriptors[i] = m_rtvHeap->Allocate();
        m_device->GetDevice()->CreateRenderTargetView(m_buffers[i], nullptr, m_descriptors[i].CPU);

        m_textures[i] = std::make_shared<Texture>(m_device);
        m_textures[i]->m_resource.Resource = m_buffers[i];
        m_textures[i]->m_rtv = m_descriptors[i];
        m_textures[i]->m_format = TextureFormat::RGBA8;
        m_textures[i]->SetState(D3D12_RESOURCE_STATE_COMMON);

        m_textures[i]->m_name = "BackBuffer" + std::to_string(i);
        std::wstring backBufferName = L"BackBuffer" + std::to_wstring(i);
        m_buffers[i]->SetName(backBufferName.c_str());
    }

    LOG(Debug, "Swap Chain created !");
}

SwapChain::~SwapChain()
{
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_buffers[i]->Release();
        m_rtvHeap->Free(m_descriptors[i]);
    }
    m_swapChain->Release();
}

void SwapChain::Present(bool vsync)
{
    HRESULT hr = m_swapChain->Present(vsync, 0);
    if(FAILED(hr))
    {
        LOG(Error, "SwapChain : failed to present swap chain !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    if (m_swapChain)
    {
        for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            m_buffers[i]->Release();
            m_rtvHeap->Free(m_descriptors[i]);
            m_textures[i].reset();
        }

        HRESULT hr = m_swapChain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_UNKNOWN, 0);
        if(FAILED(hr))
        {
            LOG(Error, "SwapChain : failed to resize buffers !");
            std::string errorMsg = std::system_category().message(hr);
            LOG(Error, errorMsg);
        }

        for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
        {
            hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i]));
            if(FAILED(hr))
            {
                LOG(Error, "SwapChain : failed to get backbuffer !");
                std::string errorMsg = std::system_category().message(hr);
                LOG(Error, errorMsg);
            }

            m_descriptors[i] = m_rtvHeap->Allocate();
            m_device->GetDevice()->CreateRenderTargetView(m_buffers[i], nullptr, m_descriptors[i].CPU);

            m_textures[i] = std::make_shared<Texture>(m_device);
            m_textures[i]->m_resource.Resource = m_buffers[i];
            m_textures[i]->m_rtv = m_descriptors[i];
            m_textures[i]->m_format = TextureFormat::RGBA8;
            m_textures[i]->SetState(D3D12_RESOURCE_STATE_COMMON);

            std::wstring backBufferName = L"BackBuffer" + std::to_wstring(i);
            m_buffers[i]->SetName(backBufferName.c_str());
        }
    }
}