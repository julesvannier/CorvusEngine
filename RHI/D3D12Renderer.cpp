#include "D3D12Renderer.h"
#include <ImGui/imgui_impl_dx12.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui.h>
#include <ImGui/ImGuizmo.h>

D3D12Renderer::D3D12Renderer(HWND hwnd) : m_frameIndex(0)
{
    m_device = std::make_shared<Device>();
    m_directCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_computeCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_copyCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COPY);
    m_heaps.RtvHeap = std::make_shared<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024);
    m_heaps.ShaderHeap = std::make_shared<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1'000'000);
    m_heaps.DsvHeap = std::make_shared<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1024);
    m_heaps.SamplerHeap = std::make_shared<DescriptorHeap>(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 512);
    m_allocator = std::make_shared<Allocator>(m_device);
    m_swapChain = std::make_shared<SwapChain>(m_device, m_directCommandQueue, m_heaps.RtvHeap, hwnd);

    LOG(Debug, "Renderer Initialization Completed");

    for(int i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_commandBuffers[i] = std::make_shared<CommandList>(m_device, m_heaps, D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_frameValues[i] = 0;
    }

    m_fontDescriptor = m_heaps.ShaderHeap->Allocate();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/InriaSans-Regular.ttf", 18);
    
    ImGui::StyleColorsDark();

    // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
    ImGuiStyle& style = ImGui::GetStyle();
    style.Alpha = 1.0f;
    style.FrameRounding = 3.0f;
    style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

    for (int i = 0; i <= ImGuiCol_COUNT; i++)
    {
        ImVec4& col = style.Colors[i];
        float H, S, V;
        ImGui::ColorConvertRGBtoHSV( col.x, col.y, col.z, H, S, V );

        if( S < 0.1f )
        {
            V = 1.0f - V;
        }
        ImGui::ColorConvertHSVtoRGB( H, S, V, col.x, col.y, col.z );
        if( col.w < 1.00f )
        {
            col.w *= 1.0f /* alpha */;
        }
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(m_device->GetDevice(), FRAMES_IN_FLIGHT, DXGI_FORMAT_R8G8B8A8_UNORM, m_heaps.ShaderHeap->GetHeap(), m_fontDescriptor.CPU, m_fontDescriptor.GPU);
}

D3D12Renderer::~D3D12Renderer()
{
    WaitForGPU();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_heaps.ShaderHeap->Free(m_fontDescriptor);
}

void D3D12Renderer::Resize(uint32_t width, uint32_t height)
{
    WaitForGPU();

    if(m_swapChain)
        m_swapChain->Resize(width, height);
}

void D3D12Renderer::EndFrame()
{
    const UINT64 currentFenceValue = m_frameValues[m_frameIndex];
    m_directCommandQueue->Signal(m_directCommandQueue->GetFence(), currentFenceValue);

    m_frameIndex = m_swapChain->AcquireImage();

    if (m_directCommandQueue->GetFence()->GetCompletedValue() < m_frameValues[m_frameIndex]) {
        m_directCommandQueue->WaitForFenceValue(m_frameValues[m_frameIndex], INFINITE);
    }

    m_frameValues[m_frameIndex] = currentFenceValue + 1;
}

void D3D12Renderer::WaitForGPU()
{
    m_directCommandQueue->Signal(m_directCommandQueue->GetFence(), m_frameValues[m_frameIndex]);
    m_directCommandQueue->WaitForFenceValue(m_frameValues[m_frameIndex], 10'000'000);
    m_frameValues[m_frameIndex]++;
}

void D3D12Renderer::Present(bool vsync)
{
    m_swapChain->Present(vsync);
}

void D3D12Renderer::BeginImGuiFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void D3D12Renderer::EndImGuiFrame()
{
    auto cmdList = m_commandBuffers[m_frameIndex]->GetCommandList();

    ID3D12DescriptorHeap* pHeaps[] = { m_heaps.ShaderHeap->GetHeap() };
    cmdList->SetDescriptorHeaps(1, pHeaps);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void D3D12Renderer::ExecuteCommandBuffers(const std::vector<std::shared_ptr<CommandList>>& buffers, D3D12_COMMAND_LIST_TYPE type)
{
    if(type == D3D12_COMMAND_LIST_TYPE_DIRECT)
    {
        m_directCommandQueue->Submit(buffers);
        return;
    }

    if(type == D3D12_COMMAND_LIST_TYPE_COMPUTE)
    {
        m_computeCommandQueue->Submit(buffers);
        return;
    }

    if(type == D3D12_COMMAND_LIST_TYPE_COPY)
    {
        m_copyCommandQueue->Submit(buffers);
        return;
    }
}

VRAMStats D3D12Renderer::GetVRAMStats() const
{
    D3D12MA::Budget budget;
    m_allocator->GetAllocator()->GetBudget(&budget, nullptr);

    VRAMStats stats;
    stats.Used = budget.UsageBytes;
    stats.Total = budget.BudgetBytes;

    return stats;
}

std::shared_ptr<GraphicsPipeline> D3D12Renderer::CreateGraphicsPipeline(GraphicsPipelineSpecs& specs)
{
    return std::make_shared<GraphicsPipeline>(m_device, specs);
}

std::shared_ptr<ComputePipeline> D3D12Renderer::CreateComputePipeline(Shader& computeShader)
{
    return std::make_shared<ComputePipeline>(m_device, computeShader);
}

std::shared_ptr<Buffer> D3D12Renderer::CreateBuffer(uint64_t size, uint64_t stride, BufferType type, bool readback)
{
    return std::make_shared<Buffer>(m_allocator, size, stride, type, readback);
}

void D3D12Renderer::CreateConstantBuffer(std::shared_ptr<Buffer> buffer)
{
    buffer->CreateConstantBuffer(m_device, m_heaps.ShaderHeap);
}

void D3D12Renderer::CreateDepthView(std::shared_ptr<Texture> texture)
{
    texture->CreateDepthTarget(m_heaps.DsvHeap);
}

void D3D12Renderer::CreateShaderResourceView(std::shared_ptr<Texture> texture)
{
    texture->CreateShaderResource(m_heaps.ShaderHeap);
}

void D3D12Renderer::CreateRenderTargetView(std::shared_ptr<Texture> texture)
{
    texture->CreateRenderTarget(m_heaps.RtvHeap);
}

void D3D12Renderer::CreateUnorderedAccessView(std::shared_ptr<Texture> texture)
{
    texture->CreateUnorderedAccessView(m_heaps.ShaderHeap);    
}

Uploader D3D12Renderer::CreateUploader()
{
    return Uploader(m_device, m_heaps, m_allocator);
}

std::shared_ptr<Texture> D3D12Renderer::CreateTexture(int width, int height, TextureFormat format, TextureType type)
{
    return std::make_shared<Texture>(m_device, m_allocator, width, height, format, type);
}

std::shared_ptr<Sampler> D3D12Renderer::CreateSampler(D3D12_TEXTURE_ADDRESS_MODE addressMode, D3D12_FILTER filter)
{
    return std::make_shared<Sampler>(m_device, m_heaps.SamplerHeap, addressMode, filter);
}

std::shared_ptr<TextureCube> D3D12Renderer::LoadTextureCube(const std::wstring& filePath)
{
    auto cmdList = std::make_shared<CommandList>(m_device, m_heaps, D3D12_COMMAND_LIST_TYPE_DIRECT);
    cmdList->Begin();
    
    auto TexCube = std::make_shared<TextureCube>(m_device, cmdList, filePath, m_heaps);
    
    cmdList->End();
    ExecuteCommandBuffers({ cmdList }, D3D12_COMMAND_LIST_TYPE_DIRECT);
    WaitForGPU();
    
    return TexCube;
}

std::shared_ptr<TextureCube> D3D12Renderer::CreateTextureCube(uint32_t width, uint32_t height, TextureFormat format)
{
    return std::make_shared<TextureCube>(m_device, m_allocator, width, height, format, m_heaps);
}

std::shared_ptr<CommandList> D3D12Renderer::CreateGraphicsCommandList()
{
    return std::make_shared<CommandList>(m_device, m_heaps, D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void D3D12Renderer::FlushUploader(Uploader& uploader)
{
    uploader.m_commandList->Begin();

    for(auto& command : uploader.m_commands)
    {
        auto cmdList = uploader.m_commandList;

        switch (command.type) {
            case Uploader::UploadCommandType::HostToDeviceShared: {
                void *pData;
                command.destBuffer->Map(0, 0, &pData);
                memcpy(pData, command.data, command.size);
                command.destBuffer->Unmap(0, 0);
                break;
            }
            case Uploader::UploadCommandType::BufferToBuffer:
            case Uploader::UploadCommandType::HostToDeviceLocal: {
                cmdList->CopyBufferToBuffer(command.destBuffer, command.sourceBuffer);
                break;
            }
            case Uploader::UploadCommandType::TextureToTexture: {
                cmdList->CopyTextureToTexture(command.destTexture, command.sourceTexture);
                break;
            }
            case Uploader::UploadCommandType::BufferToTexture: {
                auto state = command.destTexture->GetState();
                cmdList->ImageBarrier(command.destTexture, D3D12_RESOURCE_STATE_COPY_DEST);
                cmdList->CopyBufferToTexture(command.destTexture, command.sourceBuffer);
                cmdList->ImageBarrier(command.destTexture, state);
                break;                      
            }
        }
    }

    uploader.m_commandList->End();
    ExecuteCommandBuffers({ uploader.m_commandList }, D3D12_COMMAND_LIST_TYPE_DIRECT);
    WaitForGPU();
    uploader.m_commands.clear();
}
