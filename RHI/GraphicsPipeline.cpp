#include "GraphicsPipeline.h"

#include <algorithm>
#include <array>
#include <d3d12shader.h>
#include <dxcapi.h>

GraphicsPipeline::GraphicsPipeline(std::shared_ptr<Device> device, GraphicsPipelineSpecs &specs)
{
    Shader& vertexBytecode = specs.ShadersBytecodes[ShaderType::Vertex];
    Shader& fragmentBytecode = specs.ShadersBytecodes[ShaderType::Pixel];

    D3D12_SHADER_DESC VertexDesc = {};
    D3D12_SHADER_DESC PixelDesc = {};

    std::array<D3D12_ROOT_PARAMETER, 64> Parameters;
    int ParameterCount = 0;

    std::array<D3D12_DESCRIPTOR_RANGE, 64> Ranges;
    int RangeCount = 0;

    std::array<D3D12_SHADER_INPUT_BIND_DESC, 64> ShaderBinds;
    int BindCount = 0;

    ID3D12ShaderReflection* vertexReflection = ShaderCompiler::GetReflection(vertexBytecode, &VertexDesc);
    ID3D12ShaderReflection* pixelReflection = ShaderCompiler::GetReflection(fragmentBytecode, &PixelDesc);

    D3D12_SHADER_DESC HullDesc = {};
    D3D12_SHADER_DESC DomainDesc = {};
    
    ID3D12ShaderReflection* hullReflection = nullptr;
    ID3D12ShaderReflection* domainReflection = nullptr;
    
    if (specs.TessellationEnable)
    {
        Shader& hullByteCode = specs.ShadersBytecodes[ShaderType::Hull];
        Shader& domainByteCode = specs.ShadersBytecodes[ShaderType::Domain];

        hullReflection = ShaderCompiler::GetReflection(hullByteCode, &HullDesc);
        domainReflection = ShaderCompiler::GetReflection(domainByteCode, &DomainDesc);
    }

    for(int BoundResourceIndex = 0; BoundResourceIndex < VertexDesc.BoundResources; BoundResourceIndex++)
    {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
        vertexReflection->GetResourceBindingDesc(BoundResourceIndex, &ShaderInputBindDesc);
        ShaderBinds[BindCount] = ShaderInputBindDesc;
        BindCount++;
    }

    for(int BoundResourceIndex = 0; BoundResourceIndex < PixelDesc.BoundResources; BoundResourceIndex++)
    {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
        pixelReflection->GetResourceBindingDesc(BoundResourceIndex, &ShaderInputBindDesc);
        ShaderBinds[BindCount] = ShaderInputBindDesc;
        BindCount++;
    }

    if (specs.TessellationEnable)
    {
        for(int BoundResourceIndex = 0; BoundResourceIndex < HullDesc.BoundResources; BoundResourceIndex++)
        {
            D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
            hullReflection->GetResourceBindingDesc(BoundResourceIndex, &ShaderInputBindDesc);
            ShaderBinds[BindCount] = ShaderInputBindDesc;
            BindCount++;
        }

        for(int BoundResourceIndex = 0; BoundResourceIndex < DomainDesc.BoundResources; BoundResourceIndex++)
        {
            D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = {};
            domainReflection->GetResourceBindingDesc(BoundResourceIndex, &ShaderInputBindDesc);
            ShaderBinds[BindCount] = ShaderInputBindDesc;
            BindCount++;
        }
    }

    std::sort(ShaderBinds.begin(), ShaderBinds.begin() + BindCount, ShaderCompiler::CompareShaderInput);
    std::vector<LPCSTR> processedBinds;

    for(int ShaderBindIndex = 0; ShaderBindIndex < BindCount; ShaderBindIndex++)
    {
        D3D12_SHADER_INPUT_BIND_DESC ShaderInputBindDesc = ShaderBinds[ShaderBindIndex];

        bool alrdyProcessed = false;
        for(auto processedBindName : processedBinds)
        {
            if(strcmp(processedBindName, ShaderInputBindDesc.Name) == 0)
            {
                alrdyProcessed = true;
                break;
            }
        }
        
        if(alrdyProcessed)
            continue;
        
        processedBinds.emplace_back(ShaderInputBindDesc.Name);

        bool isStructuredBuffer = ShaderInputBindDesc.Type == D3D_SIT_STRUCTURED;
        
        D3D12_ROOT_PARAMETER RootParameter = {};
        RootParameter.ParameterType = isStructuredBuffer ? D3D12_ROOT_PARAMETER_TYPE_SRV : D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

        if(!isStructuredBuffer)
        {
            D3D12_DESCRIPTOR_RANGE Range = {};
            Range.NumDescriptors = 1;
            Range.BaseShaderRegister = ShaderInputBindDesc.BindPoint;
            Range.RegisterSpace = ShaderInputBindDesc.Space;

            switch (ShaderInputBindDesc.Type)
            {
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
            default:
                LOG(Error, "GraphicsPipeline : unsupported shader resource !");
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
    RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

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
    if(FAILED(hr))
    {
        LOG(Error, "GraphicsPipeline : failed to create root signature !");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
    }
    pRootSignatureBlob->Release();

    D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};
    Desc.VS.pShaderBytecode = vertexBytecode.Bytecode.data();
    Desc.VS.BytecodeLength = vertexBytecode.Bytecode.size() * sizeof(uint32_t);
    Desc.PS.pShaderBytecode = fragmentBytecode.Bytecode.data();
    Desc.PS.BytecodeLength = fragmentBytecode.Bytecode.size() * sizeof(uint32_t);

    if (specs.TessellationEnable)
    {
        Shader& hullByteCode = specs.ShadersBytecodes[ShaderType::Hull];
        Shader& domainByteCode = specs.ShadersBytecodes[ShaderType::Domain];
        
        Desc.HS.pShaderBytecode = hullByteCode.Bytecode.data();
        Desc.HS.BytecodeLength = hullByteCode.Bytecode.size() * sizeof(uint32_t);
        Desc.DS.pShaderBytecode = domainByteCode.Bytecode.data();
        Desc.DS.BytecodeLength = domainByteCode.Bytecode.size() * sizeof(uint32_t);
    }

    for (int RTVIndex = 0; RTVIndex < specs.FormatCount; RTVIndex++)
    {
        if(specs.BlendOperation != None)
        {
            Desc.BlendState.RenderTarget[RTVIndex].BlendEnable = true;
            Desc.BlendState.RenderTarget[RTVIndex].LogicOpEnable = false;
        }

        Desc.BlendState.RenderTarget[RTVIndex].SrcBlend = specs.BlendOperation == Transparency ? D3D12_BLEND_SRC_ALPHA : D3D12_BLEND_ONE;
        Desc.BlendState.RenderTarget[RTVIndex].DestBlend = specs.BlendOperation == Transparency ? D3D12_BLEND_INV_SRC_ALPHA
        : specs.BlendOperation == Additive ? D3D12_BLEND_ONE : D3D12_BLEND_ZERO;
        Desc.BlendState.RenderTarget[RTVIndex].BlendOp = D3D12_BLEND_OP_ADD;
        Desc.BlendState.RenderTarget[RTVIndex].SrcBlendAlpha = D3D12_BLEND_ONE;
        Desc.BlendState.RenderTarget[RTVIndex].DestBlendAlpha =specs.BlendOperation == Transparency ? D3D12_BLEND_INV_SRC_ALPHA : D3D12_BLEND_ZERO;
        Desc.BlendState.RenderTarget[RTVIndex].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        Desc.BlendState.RenderTarget[RTVIndex].LogicOp = D3D12_LOGIC_OP_NOOP;
        Desc.BlendState.RenderTarget[RTVIndex].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        Desc.RTVFormats[RTVIndex] = DXGI_FORMAT(specs.Formats[RTVIndex]);
        Desc.NumRenderTargets = specs.FormatCount;
    }
    
    Desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    Desc.RasterizerState.FillMode = D3D12_FILL_MODE(specs.Fill);
    Desc.RasterizerState.CullMode = D3D12_CULL_MODE(specs.Cull);
    Desc.RasterizerState.FrontCounterClockwise = specs.DepthEnabled;
    Desc.PrimitiveTopologyType = specs.Topology == Patch ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    Desc.DepthStencilState.DepthEnable = specs.DepthEnabled;
    Desc.RasterizerState.DepthClipEnable = specs.DepthEnabled;
    if(specs.DepthEnabled)
    {
        Desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        Desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC(specs.Depth);
        Desc.DSVFormat = DXGI_FORMAT(specs.DepthFormat);
    }
    
    Desc.SampleDesc.Count = 1;

    std::vector<D3D12_INPUT_ELEMENT_DESC> InputElementDescs;
    std::vector<std::string> InputElementSemanticNames;
    
    InputElementSemanticNames.reserve(VertexDesc.InputParameters);
    InputElementDescs.reserve(VertexDesc.InputParameters);

    for (int ParameterIndex = 0; ParameterIndex < VertexDesc.InputParameters; ParameterIndex++)
    {
        D3D12_SIGNATURE_PARAMETER_DESC ParameterDesc = {};
        vertexReflection->GetInputParameterDesc(ParameterIndex, &ParameterDesc);

        InputElementSemanticNames.push_back(ParameterDesc.SemanticName);

        D3D12_INPUT_ELEMENT_DESC InputElement = {};
        InputElement.SemanticName = InputElementSemanticNames.back().c_str();
        InputElement.SemanticIndex = ParameterDesc.SemanticIndex;
        InputElement.InputSlot = 0;
        InputElement.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        InputElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        InputElement.InstanceDataStepRate = 0;

        if (ParameterDesc.Mask == 1)
        {
            if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) InputElement.Format = DXGI_FORMAT_R32_UINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) InputElement.Format = DXGI_FORMAT_R32_SINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputElement.Format = DXGI_FORMAT_R32_FLOAT;
        }
        else if (ParameterDesc.Mask <= 3)
        {
            if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) InputElement.Format = DXGI_FORMAT_R32G32_UINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) InputElement.Format = DXGI_FORMAT_R32G32_SINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputElement.Format = DXGI_FORMAT_R32G32_FLOAT;
        }
        else if (ParameterDesc.Mask <= 7)
        {
            if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) InputElement.Format = DXGI_FORMAT_R32G32B32_UINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) InputElement.Format = DXGI_FORMAT_R32G32B32_SINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputElement.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        }
        else if (ParameterDesc.Mask <= 15)
        {
            if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) InputElement.Format = DXGI_FORMAT_R32G32B32A32_UINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) InputElement.Format = DXGI_FORMAT_R32G32B32A32_SINT;
            else if (ParameterDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) InputElement.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        }

        InputElementDescs.push_back(InputElement);
    }
    Desc.InputLayout.pInputElementDescs = InputElementDescs.data();
    Desc.InputLayout.NumElements = static_cast<uint32_t>(InputElementDescs.size());
    Desc.pRootSignature = m_rootSignature;

    hr = device->GetDevice()->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(&m_pipelineState));
    if(FAILED(hr))
    {
        LOG(Error, "GraphicsPipeline: Failed to create graphics pipeline!");
        std::string errorMsg = std::system_category().message(hr);
        LOG(Error, errorMsg);
        return;
    }

    pixelReflection->Release();
    vertexReflection->Release();

    LOG(Debug, "GraphicsPipeline: Created a Graphics Pipeline !");
}

GraphicsPipeline::~GraphicsPipeline()
{
    m_rootSignature->Release();
    m_pipelineState->Release();
}