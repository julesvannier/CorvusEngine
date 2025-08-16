#pragma once
#include <Core.h>

#include "Texture.h"
#include "../Rendering/ShaderCompiler.h"

enum class FillMode
{
    Solid = D3D12_FILL_MODE_SOLID,
    Line = D3D12_FILL_MODE_WIREFRAME
};

enum class CullMode
{
    Back = D3D12_CULL_MODE_BACK,
    Front = D3D12_CULL_MODE_FRONT,
    None = D3D12_CULL_MODE_NONE
};

enum class DepthOperation
{
    Greater = D3D12_COMPARISON_FUNC_GREATER,
    Less = D3D12_COMPARISON_FUNC_LESS,
    Equal = D3D12_COMPARISON_FUNC_EQUAL,
    LEqual = D3D12_COMPARISON_FUNC_LESS_EQUAL,
    None = D3D12_COMPARISON_FUNC_NEVER
};

enum BlendOperation
{
    None,
    Transparency,
    Additive
};

enum TopologyType
{
    Triangle,
    Patch
};

struct GraphicsPipelineSpecs
{
    FillMode Fill;
    CullMode Cull;
    DepthOperation Depth;

    TextureFormat Formats[32];
    int FormatCount;
    TextureFormat DepthFormat;
    bool DepthEnabled;
    BlendOperation BlendOperation;
    
    TopologyType Topology = Triangle;
    bool TessellationEnable = false;

    std::unordered_map<ShaderType, Shader> ShadersBytecodes;
};

class GraphicsPipeline
{
public:
    GraphicsPipeline(std::shared_ptr<Device> device, GraphicsPipelineSpecs& specs);
    ~GraphicsPipeline();

    ID3D12PipelineState* GetPipelineState() { return m_pipelineState; }
    ID3D12RootSignature* GetRootSignature() { return m_rootSignature; }

private:
    ID3D12PipelineState* m_pipelineState;
    ID3D12RootSignature* m_rootSignature;
};