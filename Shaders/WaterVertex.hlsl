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
};

struct VertexOut
{
    float4 Position : SV_POSITION;
};

VertexOut Main(VertexIn Input, uint InstanceID : SV_InstanceID)
{
    VertexOut Output;
    InstanceData instanceData = InstancesData[InstanceID];
    Output.Position = mul(float4(Input.position, 1.0), instanceData.WorldMat);
    return Output;
}