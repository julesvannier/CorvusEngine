SamplerState Sampler : register(s1);
Texture2D Albedo : register(t2);
Texture2D Normal : register(t3);
Texture2D MetallicRoughness : register(t4);

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
};

struct PixelOut
{
    float3 Albedo : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float3 MetallicRoughness : SV_TARGET2;
};

PixelOut Main(PixelIn Input)
{
    PixelOut pixelOut;
    
    float4 albedo = float4(1.0, 1.0, 1.0, 1.0);
    float3 metallicRoughness = float3(0.0f, 0.15f, 0.0f); // G roughness, B metallic
    float3 normal = Input.normal;

    if(Input.HasAlbedo)
        albedo = Albedo.Sample(Sampler, Input.uv);

    if(Input.HasNormalMap)
    {
        float4 normalSampled = Normal.Sample(Sampler, Input.uv);
        normalSampled.xyz = (normalSampled.xyz * 2.0) - 1.0;
        normalSampled.xyz = mul(normalSampled.xyz, Input.tbn);
        normal = normalSampled.xyz;
    }

    if(Input.HasMetallicRoughness)
    {
        float3 mr = MetallicRoughness.Sample(Sampler, Input.uv).rgb;
        metallicRoughness = float3(0.0f, mr.g, mr.b); 
    }

    normal = normalize(normal);

    pixelOut.Albedo = albedo.xyz;
    pixelOut.Normal = float4(normal, 1.0);
    pixelOut.MetallicRoughness = metallicRoughness;

    return pixelOut;
}