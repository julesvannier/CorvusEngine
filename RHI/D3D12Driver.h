#pragma once
#include "Core.h"
#include "Device.h"
#include "Allocator.h"
#include "Buffer.h"
#include "CommandList.h"
#include "CommandQueue.h"
#include "ComputePipeline.h"
#include "DescriptorHeap.h"
#include "GraphicsPipeline.h"
#include "SwapChain.h"
#include "Sampler.h"
#include "TextureCube.h"

struct Image;

struct VRAMStats
{
    uint64_t Used;
    uint64_t Total;
};

class D3D12Driver
{
public:
    D3D12Driver(HWND hwnd);
    ~D3D12Driver();

    void Resize(uint32_t width, uint32_t height);
    void EndFrame();
    void Present(bool vsync);

    void BeginImGuiFrame();
    void EndImGuiFrame();

    void ExecuteCommandBuffers(const std::vector<std::shared_ptr<CommandList>>& buffers, D3D12_COMMAND_LIST_TYPE type);

    std::shared_ptr<CommandList> GetCurrentCommandList() { return m_commandBuffers[m_frameIndex]; }
    std::shared_ptr<Texture> GetBackBuffer() { return m_swapChain->GetTexture(m_frameIndex); }
    VRAMStats GetVRAMStats() const;

    std::shared_ptr<GraphicsPipeline> CreateGraphicsPipeline(GraphicsPipelineSpecs& specs);
    std::shared_ptr<ComputePipeline> CreateComputePipeline(Shader& computeShader);
    std::shared_ptr<Buffer> CreateBuffer(uint64_t size, uint64_t stride, BufferType type, bool readback);
    void CreateConstantBuffer(std::shared_ptr<Buffer> buffer);
    void CreateDepthView(std::shared_ptr<Texture> texture);
    void CreateShaderResourceView(std::shared_ptr<Texture> texture);
    void CreateRenderTargetView(std::shared_ptr<Texture> texture);
    void CreateUnorderedAccessView(std::shared_ptr<Texture> texture);
    std::shared_ptr<Texture> CreateTexture(int width, int height, TextureFormat format, TextureType type);
    std::shared_ptr<Sampler> CreateSampler(D3D12_TEXTURE_ADDRESS_MODE addressMode, D3D12_FILTER filter);
    std::shared_ptr<TextureCube> LoadTextureCube(const std::wstring& filePath);
    std::shared_ptr<TextureCube> CreateTextureCube(uint32_t width, uint32_t height, TextureFormat format);
    std::shared_ptr<CommandList> CreateGraphicsCommandList();

    void UploadBufferData(void* pData, uint64_t size, std::shared_ptr<Buffer> destBuffer);
    void UploadTextureData(Image& image, std::shared_ptr<Texture> destTexture);
    void CopyBufferToBuffer(std::shared_ptr<Buffer> sourceBuffer, std::shared_ptr<Buffer> destBuffer);
    void CopyTextureToTexture(std::shared_ptr<Texture> sourceTexture, std::shared_ptr<Texture> destTexture);
    bool HasPendingUploads() const { return !m_uploadCommands.empty(); }
    void FlushUploads();

    void WaitForGPU();

private:
    enum class UploadCommandType
    {
        HostToDeviceShared,
        HostToDeviceLocal,
        BufferToBuffer,
        TextureToTexture,
        BufferToTexture
    };

    struct UploadCommand
    {
        UploadCommandType type;
        void* data;
        uint64_t size;

        std::shared_ptr<Texture> sourceTexture;
        std::shared_ptr<Texture> destTexture;

        std::shared_ptr<Buffer> sourceBuffer;
        std::shared_ptr<Buffer> destBuffer;
    };

    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandQueue> m_directCommandQueue;
    std::shared_ptr<CommandQueue> m_computeCommandQueue;
    std::shared_ptr<CommandQueue> m_copyCommandQueue;
    std::shared_ptr<Allocator> m_allocator;
    std::shared_ptr<SwapChain> m_swapChain;
    Heaps m_heaps;

    uint64_t m_frameIndex;
    uint64_t m_frameValues[FRAMES_IN_FLIGHT];
    std::shared_ptr<CommandList> m_commandBuffers[FRAMES_IN_FLIGHT];

    std::shared_ptr<CommandList> m_uploadCommandList;
    std::vector<UploadCommand> m_uploadCommands;

    DescriptorHandle m_fontDescriptor;
};
