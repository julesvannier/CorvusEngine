// SamplerState Sampler : register(s1);
// Texture2D Albedo : register(t2);
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
    // safety Yellow
    return float4(255.0f, 240.0f, 0.0f, OpacityData[Input.instanceID]);
}