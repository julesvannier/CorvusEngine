cbuffer CBuf : register(b0)
{
    row_major float4x4 ViewProj;
    float Time;
    float3 CameraPosition;
    int Mode;
    float3 DirLightDirection;
    float DirLightIntensity;
    float3 Padding;
    row_major float4x4 InvViewProj;
    row_major float4x4 ShadowTransform;
    bool ShadowEnabled;
    float3 Padding2;
};

struct InstanceData
{
    row_major float4x4 WorldMat;
    bool HasAlbedo;
    bool HasNormalMap;
    bool HasMetallicRoughness;
};

StructuredBuffer<InstanceData> InstancesData : register(t1, space1);

struct VertexIn
{
    float3 position : POSITION;
    float2 texcoord : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

struct VertexOut
{
    float4 Position : SV_POSITION;
    float3 PositionWS : TEXCOORD0;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD1;
    bool HasAlbedo : TEXCOORD2;
    bool HasNormalMap : TEXCOORD3;
    bool HasMetallicRoughness : TEXCOORD4;
    row_major float3x3 tbn : TEXCOORD5;
};

VertexOut Main(VertexIn Input, uint InstanceID : SV_InstanceID)
{
    VertexOut Output;
    InstanceData instanceData = InstancesData[InstanceID];
    Output.PositionWS = mul(float4(Input.position, 1.0), instanceData.WorldMat).xyz;
    Output.Position = mul(float4(Output.PositionWS, 1.0), ViewProj);
    Output.normal = normalize(mul(Input.normal, (float3x3)instanceData.WorldMat));
    Output.uv = Input.texcoord;
    
    Output.tbn[0] = normalize(mul(Input.tangent, (float3x3)instanceData.WorldMat));
    Output.tbn[1] = normalize(mul(Input.binormal, (float3x3)instanceData.WorldMat));
    Output.tbn[2] = normalize(mul(Input.normal, (float3x3)instanceData.WorldMat));

    Output.HasAlbedo = instanceData.HasAlbedo;
    Output.HasNormalMap = instanceData.HasNormalMap;
    Output.HasMetallicRoughness = instanceData.HasMetallicRoughness;

    return Output;
}