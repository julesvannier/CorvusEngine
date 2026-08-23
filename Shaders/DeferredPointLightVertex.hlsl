cbuffer CBuf : register(b0)
{
    row_major float4x4 View;
    row_major float4x4 Proj;
    float Time;
    float3 CameraPosition;
    int Mode;
    float3 DirLightDirection;
    float DirLightIntensity;
    float3 Padding;
    row_major float4x4 InvViewProj;
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
    float3 CameraPosition : TEXCOORD0;
    float3 ObjectPosition : TEXCOORD1;
    int InstanceIdx : TEXCOORD2;
};

VertexOut Main(VertexIn Input, uint InstanceID : SV_InstanceID)
{
    VertexOut Output;
    float3 positionWS = mul(float4(Input.position, 1.0), InstancesData[InstanceID].WorldMat).xyz;
    Output.Position = mul(mul(float4(positionWS, 1.0), View), Proj);
    Output.CameraPosition = CameraPosition;
    Output.ObjectPosition = float3(InstancesData[InstanceID].WorldMat[3].xyz);
    Output.InstanceIdx = InstanceID;

    return Output;
}