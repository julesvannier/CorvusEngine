#include "ComputePipeline.h"
#include <algorithm>

ComputePipeline::ComputePipeline(std::shared_ptr<Device> device, Shader& shader)
{
    D3D12_SHADER_DESC ComputeDesc = {};
    ID3D12ShaderReflection* pReflection = ShaderCompiler::GetReflection(shader, &ComputeDesc);

    std::array<D3D12_ROOT_PARAMETER, 64> Parameters;
    int ParameterCount = 0;

    std::array<D3D12_DESCRIPTOR_RANGE, 64> Ranges;
    int RangeCount = 0;

    std::array<D3D12_SHADER_INPUT_BIND_DESC, 64> ShaderBinds;
    int BindCount = 0;

    for (int BoundResourceIndex = 0; BoundResourceIndex < ComputeDesc.BoundResources; BoundResourceIndex++)
    {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
        pReflection->GetResourceBindingDesc(BoundResourceIndex, &ShaderInputBindDesc);
        ShaderBinds[BindCount] = ShaderInputBindDesc;
        BindCount++;
    }

    std::sort(ShaderBinds.begin(), ShaderBinds.begin() + BindCount, ShaderCompiler::CompareShaderInput);

    for (int ShaderBindIndex = 0; ShaderBindIndex < BindCount; ShaderBindIndex++) {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = ShaderBinds[ShaderBindIndex];

        bool isStructuredBuffer = ShaderInputBindDesc.Type == D3D_SIT_STRUCTURED;

        D3D12_ROOT_PARAMETER RootParameter = {};
        RootParameter.ParameterType = isStructuredBuffer ? D3D12_ROOT_PARAMETER_TYPE_SRV : D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        if (!isStructuredBuffer)
        {
            D3D12_DESCRIPTOR_RANGE Range = {};
            Range.NumDescriptors = 1;
            Range.BaseShaderRegister = ShaderInputBindDesc.BindPoint;

            switch (ShaderInputBindDesc.Type) {
            case D3D_SIT_SAMPLER:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                break;
            case D3D_SIT_TEXTURE:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                break;
            case D3D_SIT_UAV_RWTYPED:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;
            case D3D_SIT_CBUFFER:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                break;
            case D3D_SIT_UAV_RWBYTEADDRESS:
                Range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                break;
            default:
                LOG(Error, "ComputePipeline : unsupported shader resource !");
                continue;
            }

            Ranges[RangeCount] = Range;

            RootParameter.DescriptorTable.NumDescriptorRanges = 1;
            RootParameter.DescriptorTable.pDescriptorRanges = &Ranges[RangeCount];
        }
        else
        {
            RootParameter.Descriptor.ShaderRegister = ShaderInputBindDesc.BindPoint;
            RootParameter.Descriptor.RegisterSpace = ShaderInputBindDesc.Space;
        }
        
        RootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        Parameters[ParameterCount] = RootParameter;

        ParameterCount++;
        RangeCount++;
    }

    D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
    RootSignatureDesc.NumParameters = ParameterCount;
    RootSignatureDesc.pParameters = Parameters.data();
    
    ID3DBlob* pRootSignatureBlob;
    ID3DBlob* pErrorBlob;
    D3D12SerializeRootSignature(&RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &pRootSignatureBlob, &pErrorBlob);
    if (pErrorBlob)
    {
        std::string errorMessage(static_cast<const char*>(pErrorBlob->GetBufferPointer()), pErrorBlob->GetBufferSize());
        LOG(Error, errorMessage);
        pErrorBlob->Release();
    }
    
    HRESULT hr = device->GetDevice()->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
    if (FAILED(hr))
    {
        LOG(Error, "ComputePipeline : failed to create root signature !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }
    pRootSignatureBlob->Release();

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_rootSignature;
    desc.CS.pShaderBytecode = shader.Bytecode.data();
    desc.CS.BytecodeLength = shader.Bytecode.size() * sizeof(uint32_t);
    
    hr = device->GetDevice()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipelineState));
    if (FAILED(hr))
    {
        LOG(Error, "ComputePipeline : failed to compute pipeline !");
        return;
    }
    
    pReflection->Release();

    LOG(Debug, "ComputePipeline: Created a Compute Pipeline !");
}

ComputePipeline::~ComputePipeline()
{
    m_rootSignature->Release();
    m_pipelineState->Release();
}
