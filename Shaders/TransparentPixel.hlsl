SamplerState Sampler : register(s3);
Texture2D Albedo : register(t4);
// Texture2D Normal : register(t3);
// Texture2D MetallicRoughness : register(t4);

struct PixelIn
{
    float4 Position : SV_POSITION;
    float3 PositionWS : TEXCOORD0;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD1;
    bool HasAlbedo : TEXCOORD2;
    bool HasNormalMap : TEXCOORD3;
    bool HasMetallicRoughness : TEXCOORD4;
    row_major float3x3 tbn : TEXCOORD5;
    uint instanceID : INSTANCE_ID;
};

StructuredBuffer<float> OpacityData : register(t2, space2);

float4 Main(PixelIn Input) : SV_Target
{
    float4 albedo = float4(1.0, 1.0, 1.0, 1.0);
    if(Input.HasAlbedo)
        albedo = Albedo.Sample(Sampler, Input.uv);
    
    return float4(albedo.xyz, OpacityData[Input.instanceID]);
}