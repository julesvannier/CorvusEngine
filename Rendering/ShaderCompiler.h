#pragma once
#include "Core.h"
#include <d3d12shader.h>
#include <dxcapi.h>

enum class ShaderType
{
    Vertex,
    Pixel,
    Compute,
    Hull,
    Domain
};

struct Shader
{
    ShaderType Type;
    std::vector<uint32_t> Bytecode;
};

class ShaderCompiler 
{
public:
    static void CompileShader(const std::string& path, ShaderType type, Shader& outShader);
    static ID3D12ShaderReflection* GetReflection(Shader& bytecode, D3D12_SHADER_DESC *desc);
    static bool CompareShaderInput(const D3D12_SHADER_INPUT_BIND_DESC& A, const D3D12_SHADER_INPUT_BIND_DESC& B);
};
