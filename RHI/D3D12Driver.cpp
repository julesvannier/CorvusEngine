#include "D3D12Driver.h"
#include "Image.h"
#include <ImGui/imgui_impl_dx12.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui.h>
#include <ImGui/ImGuizmo.h>

D3D12Driver::D3D12Driver(HWND hwnd) : m_frameIndex(0)
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

    LOG(Debug, "Driver Initialization Completed");

    for(int i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_commandBuffers[i] = std::make_shared<CommandList>(m_device, m_heaps, D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_frameValues[i] = 0;
    }

    m_uploadCommandList = std::make_shared<CommandList>(m_device, m_heaps, D3D12_COMMAND_LIST_TYPE_DIRECT);

    m_fontDescriptor = m_heaps.ShaderHeap->Allocate();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/InriaSans-Regular.ttf", 24);

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

D3D12Driver::~D3D12Driver()
{
    WaitForGPU();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    m_heaps.ShaderHeap->Free(m_fontDescriptor);
}

void D3D12Driver::Resize(uint32_t width, uint32_t height)
{
    WaitForGPU();

    if(m_swapChain)
        m_swapChain->Resize(width, height);
}

void D3D12Driver::EndFrame()
{
    const UINT64 currentFenceValue = m_frameValues[m_frameIndex];
    m_directCommandQueue->Signal(m_directCommandQueue->GetFence(), currentFenceValue);

    m_frameIndex = m_swapChain->AcquireImage();

    if (m_directCommandQueue->GetFence()->GetCompletedValue() < m_frameValues[m_frameIndex]) {
        m_directCommandQueue->WaitForFenceValue(m_frameValues[m_frameIndex], INFINITE);
    }

    m_frameValues[m_frameIndex] = currentFenceValue + 1;
}

void D3D12Driver::WaitForGPU()
{
    m_directCommandQueue->Signal(m_directCommandQueue->GetFence(), m_frameValues[m_frameIndex]);
    m_directCommandQueue->WaitForFenceValue(m_frameValues[m_frameIndex], 10'000'000);
    m_frameValues[m_frameIndex]++;
}

void D3D12Driver::Present(bool vsync)
{
    m_swapChain->Present(vsync);
}

void D3D12Driver::BeginImGuiFrame()
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();
}

void D3D12Driver::EndImGuiFrame()
{
    auto cmdList = m_commandBuffers[m_frameIndex]->GetCommandList();

    ID3D12DescriptorHeap* pHeaps[] = { m_heaps.ShaderHeap->GetHeap() };
    cmdList->SetDescriptorHeaps(1, pHeaps);

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
}

void D3D12Driver::ExecuteCommandBuffers(const std::vector<std::shared_ptr<CommandList>>& buffers, D3D12_COMMAND_LIST_TYPE type)
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

VRAMStats D3D12Driver::GetVRAMStats() const
{
    D3D12MA::Budget budget;
    m_allocator->GetAllocator()->GetBudget(&budget, nullptr);

    VRAMStats stats;
    stats.Used = budget.UsageBytes;
    stats.Total = budget.BudgetBytes;

    return stats;
}

std::shared_ptr<GraphicsPipeline> D3D12Driver::CreateGraphicsPipeline(GraphicsPipelineSpecs& specs)
{
    return std::make_shared<GraphicsPipeline>(m_device, specs);
}

std::shared_ptr<ComputePipeline> D3D12Driver::CreateComputePipeline(Shader& computeShader)
{
    return std::make_shared<ComputePipeline>(m_device, computeShader);
}

std::shared_ptr<Buffer> D3D12Driver::CreateBuffer(uint64_t size, uint64_t stride, BufferType type, bool readback)
{
    return std::make_shared<Buffer>(m_allocator, size, stride, type, readback);
}

void D3D12Driver::CreateConstantBuffer(std::shared_ptr<Buffer> buffer)
{
    buffer->CreateConstantBuffer(m_device, m_heaps.ShaderHeap);
}

void D3D12Driver::CreateDepthView(std::shared_ptr<Texture> texture)
{
    texture->CreateDepthTarget(m_heaps.DsvHeap);
}

void D3D12Driver::CreateShaderResourceView(std::shared_ptr<Texture> texture)
{
    texture->CreateShaderResource(m_heaps.ShaderHeap);
}

void D3D12Driver::CreateRenderTargetView(std::shared_ptr<Texture> texture)
{
    texture->CreateRenderTarget(m_heaps.RtvHeap);
}

void D3D12Driver::CreateUnorderedAccessView(std::shared_ptr<Texture> texture)
{
    texture->CreateUnorderedAccessView(m_heaps.ShaderHeap);
}

std::shared_ptr<Texture> D3D12Driver::CreateTexture(int width, int height, TextureFormat format, TextureType type, const char* name)
{
    return std::make_shared<Texture>(m_device, m_allocator, width, height, format, type, name);
}

std::shared_ptr<Sampler> D3D12Driver::CreateSampler(D3D12_TEXTURE_ADDRESS_MODE addressMode, D3D12_FILTER filter)
{
    return std::make_shared<Sampler>(m_device, m_heaps.SamplerHeap, addressMode, filter);
}

std::shared_ptr<TextureCube> D3D12Driver::LoadTextureCube(const std::wstring& filePath)
{
    auto cmdList = std::make_shared<CommandList>(m_device, m_heaps, D3D12_COMMAND_LIST_TYPE_DIRECT);
    cmdList->Begin();

    auto TexCube = std::make_shared<TextureCube>(m_device, cmdList, filePath, m_heaps);

    cmdList->End();
    ExecuteCommandBuffers({ cmdList }, D3D12_COMMAND_LIST_TYPE_DIRECT);
    WaitForGPU();

    return TexCube;
}

std::shared_ptr<TextureCube> D3D12Driver::CreateTextureCube(uint32_t width, uint32_t height, TextureFormat format, const char* name)
{
    return std::make_shared<TextureCube>(m_device, m_allocator, width, height, format, m_heaps, name);
}

std::shared_ptr<CommandList> D3D12Driver::CreateGraphicsCommandList()
{
    return std::make_shared<CommandList>(m_device, m_heaps, D3D12_COMMAND_LIST_TYPE_DIRECT);
}

void D3D12Driver::UploadBufferData(void* pData, uint64_t size, std::shared_ptr<Buffer> destBuffer)
{
    if (destBuffer->IsUploadHeap())
    {
        UploadCommand command;
        command.type = UploadCommandType::HostToDeviceShared;
        command.data = pData;
        command.size = size;
        command.destBuffer = destBuffer;

        m_uploadCommands.push_back(command);
        return;
    }

    auto stagingBuffer = std::make_shared<Buffer>(m_allocator, size, 0, BufferType::Copy, false);

    {
        UploadCommand command;
        command.type = UploadCommandType::HostToDeviceShared;
        command.data = pData;
        command.size = size;
        command.destBuffer = stagingBuffer;

        m_uploadCommands.push_back(command);
    }

    {
        UploadCommand command;
        command.type = UploadCommandType::HostToDeviceLocal;
        command.sourceBuffer = stagingBuffer;
        command.destBuffer = destBuffer;

        m_uploadCommands.push_back(command);
    }
}

void D3D12Driver::UploadTextureData(Image& image, std::shared_ptr<Texture> destTexture)
{
    uint32_t bpp = GetFormatBytesPerPixel(destTexture->GetFormat());
    uint32_t srcRowPitch = image.Width * bpp;
    uint32_t dstRowPitch = GetTextureRowPitch(destTexture->GetFormat(), image.Width);

    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>(m_allocator, (uint64_t)dstRowPitch * image.Height, 0, BufferType::Copy, false);

    {
        UploadCommand command;
        command.type = UploadCommandType::HostToDeviceShared;
        command.data = image.Bytes;
        command.size = (uint64_t)srcRowPitch * image.Height;
        command.destBuffer = buffer;
        command.rowPitchSrc = srcRowPitch;
        command.rowPitchDst = dstRowPitch;
        command.rowCount = (uint32_t)image.Height;

        m_uploadCommands.push_back(command);
    }

    {
        UploadCommand command;
        command.type = UploadCommandType::BufferToTexture;
        command.sourceBuffer = buffer;
        command.destTexture = destTexture;

        m_uploadCommands.push_back(command);
    }
}

void D3D12Driver::CopyBufferToBuffer(std::shared_ptr<Buffer> sourceBuffer, std::shared_ptr<Buffer> destBuffer)
{
    UploadCommand command;
    command.type = UploadCommandType::BufferToBuffer;
    command.sourceBuffer = sourceBuffer;
    command.destBuffer = destBuffer;

    m_uploadCommands.push_back(command);
}

void D3D12Driver::CopyTextureToTexture(std::shared_ptr<Texture> sourceTexture, std::shared_ptr<Texture> destTexture)
{
    UploadCommand command;
    command.type = UploadCommandType::TextureToTexture;
    command.sourceTexture = sourceTexture;
    command.destTexture = destTexture;

    m_uploadCommands.push_back(command);
}

void D3D12Driver::FlushUploads()
{
    m_uploadCommandList->Begin();

    for(auto& command : m_uploadCommands)
    {
        auto cmdList = m_uploadCommandList;

        switch (command.type) {
            case UploadCommandType::HostToDeviceShared: {
                void *pData;
                command.destBuffer->Map(0, 0, &pData);
                if (command.rowCount > 0 && command.rowPitchDst != command.rowPitchSrc)
                {
                    uint8_t* dst = reinterpret_cast<uint8_t*>(pData);
                    const uint8_t* src = reinterpret_cast<const uint8_t*>(command.data);
                    for (uint32_t row = 0; row < command.rowCount; row++)
                        memcpy(dst + (uint64_t)row * command.rowPitchDst, src + (uint64_t)row * command.rowPitchSrc, command.rowPitchSrc);
                }
                else
                {
                    memcpy(pData, command.data, command.size);
                }
                command.destBuffer->Unmap(0, 0);
                break;
            }
            case UploadCommandType::BufferToBuffer:
            case UploadCommandType::HostToDeviceLocal: {
                cmdList->CopyBufferToBuffer(command.destBuffer, command.sourceBuffer);
                break;
            }
            case UploadCommandType::TextureToTexture: {
                cmdList->CopyTextureToTexture(command.destTexture, command.sourceTexture);
                break;
            }
            case UploadCommandType::BufferToTexture: {
                auto state = command.destTexture->GetState();
                cmdList->ImageBarrier(command.destTexture, D3D12_RESOURCE_STATE_COPY_DEST);
                cmdList->CopyBufferToTexture(command.destTexture, command.sourceBuffer);
                cmdList->ImageBarrier(command.destTexture, state);
                break;
            }
        }
    }

    m_uploadCommandList->End();
    ExecuteCommandBuffers({ m_uploadCommandList }, D3D12_COMMAND_LIST_TYPE_DIRECT);
    WaitForGPU();
    m_uploadCommands.clear();
}
