#include "Shaders/PBR.hlsl"

struct VertexOut
{
    float4 Position : SV_POSITION;
    float2 Texcoord : TEXCOORD;
};

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
    row_major float4x4 ShadowTransform;
    bool ShadowEnabled;
    float3 Padding2;
};

SamplerState Sampler : register(s1);
Texture2D Albedo : register(t2);
Texture2D Normal : register(t3);
Texture2D MetallicRoughness : register(t4);
Texture2D Depth : register(t5);
TextureCube Irradiance : register(t6);
TextureCube PrefilterEnvMap : register(t7);
// Texture2D BRDFLut : register(t8);
Texture2D ShadowMap : register(t8);
SamplerComparisonState CmpSampler : register(s9);

float CalcShadowFactor(float4 shadowPos, float3 normal, float3 lightDir)
{
    // Complete projection by doing division by w.
    shadowPos.xyz /= shadowPos.w;

    if(shadowPos.z > 1.0)
        return 0.0f;

    // Depth in NDC space.
    float depth = shadowPos.z;

    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.002);

    uint width, height, numMips;
    ShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float)width;

    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    float percentLit = 0.0f;
    for(int i = 0; i < 9; ++i)
    {
        float pcf = ShadowMap.SampleCmpLevelZero(CmpSampler, shadowPos.xy + offsets[i], depth).r;
        percentLit += depth - bias > pcf ? 1.0 : 0.0f;
    }

    return 1.0f - (percentLit / 9.0f);
}

float4 Main(VertexOut Input) : SV_TARGET
{
    float4 albedo = float4(Albedo.Sample(Sampler, Input.Texcoord.xy).xyz, 1.0);
    float3 normal = normalize(Normal.Sample(Sampler, Input.Texcoord.xy).xyz);
    float3 metallicRoughness = MetallicRoughness.Sample(Sampler, Input.Texcoord.xy).xyz;
    float roughness = metallicRoughness.g;
    float metallic = metallicRoughness.b;
    float depth = Depth.Sample(Sampler, Input.Texcoord.xy).x;
    if(depth == 1.0f)
        discard;
    
    float4 clipSpacePosition = float4(Input.Texcoord * 2.0 - 1.0, depth, 1.0);
    clipSpacePosition.y *= -1.0;

    float4 worldSpacePosition = mul(clipSpacePosition, InvViewProj);
    worldSpacePosition /= worldSpacePosition.w;

    float4 lightColor = float4(1.0, 1.0, 1.0, 1.0) * DirLightIntensity;
    float3 dirLightVec = normalize(DirLightDirection) * -1.0f; // l (norm vec pointing toward light direction)

    float3 view = normalize(CameraPosition - worldSpacePosition.xyz);
    float3 F0 = lerp(Fdielectric, albedo.xyz, metallic);

    float shadowFactor = 1.0f;
    if(ShadowEnabled)
    {
        float4 shadowPos = mul(worldSpacePosition, ShadowTransform);
        shadowFactor = CalcShadowFactor(shadowPos, normal, dirLightVec);
    }
    
    float3 finalLight = PBR(F0, normal, view, dirLightVec, normalize(dirLightVec + view), lightColor.xyz, albedo.xyz, roughness, metallic);

    float3 Ks = FresnelSchlickRoughness(max(dot(normal, view), 0.0f), F0,  roughness);

    float3 Kd = 1.0f - Ks;
    Kd *= 1.0 - metallic;
    float3 r = reflect(-view, normal);
    float3 irradiance = float3(Irradiance.Sample(Sampler, r).rgb);

    float3 diffuse = albedo.xyz * irradiance;

    float prefilterLOD = roughness * 4;
    float3 prefiltered = PrefilterEnvMap.SampleLevel(Sampler, r, prefilterLOD).rgb;
    
    float2 brdvUV = float2(max(dot(normal, view), 0.0), roughness);
    // float2 envBRDF = BRDFLut.Sample(Sampler, brdvUV).rg;
    
    float3 specular = prefiltered * (Ks /* * envBRDF.x + envBRDF.y */);

    float3 ambiantLight = Kd * diffuse + specular;

    float3 outLight = (ambiantLight * DirLightIntensity) + finalLight * shadowFactor;

    switch (Mode)
    {
    case 0:
        return float4(outLight, 1.0);
    case 1:
        return albedo;
    case 2:
        return float4(normal, 1.0);
    case 3:
        return float4(depth, 0.0, 0.0, 1.0);
    case 4:
        return float4(worldSpacePosition.xyz, 1.0f);
    case 5:
        return float4(metallicRoughness, 1.0);
    case 6:
        return float4(shadowFactor, shadowFactor, shadowFactor, 1.0);
    default:
        return float4(outLight, 1.0);
    }
    
    // safety Yellow
    // return float4(255.0f, 240.0f, 0.0f, 1.0f);
}