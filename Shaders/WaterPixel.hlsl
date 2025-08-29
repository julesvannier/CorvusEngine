#include "Shaders/PBR.hlsl"

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

cbuffer WaterCBuf : register(b2)
{
    float4 WaterColor;
    float WavesScalar;
    float NormalScrollSpeed;
    float NormalTilingFactor;
    float NormalTilingFactor2;
}

SamplerState Sampler : register(s3);
Texture2D NormalMap : register(t4);
Texture2D NormalMap2 : register(t5);

struct PixelIn
{
    float4 Position : SV_POSITION;
    float3 posWS : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    float2 uv : TEXCOORD1;
};

float remap(float value, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

float4 Main(PixelIn Input) : SV_Target
{
    float3 n = normalize(Input.normal);
    float3x3 tbn = float3x3(normalize(Input.tangent), normalize(Input.binormal), n);

    float2 normalMapUV = Input.uv * NormalTilingFactor + Time * float2(1.0f, 0.0f) * NormalScrollSpeed; 
    float2 normalMapUV2 = Input.uv * NormalTilingFactor2 + Time * float2(0.0f, 1.0f) * NormalScrollSpeed; 
    
    float3 normalSampled = (NormalMap.Sample(Sampler, normalMapUV).rgb * 2.0f) - 1.0f;
    float3 normal = normalize(mul(normalSampled, tbn));
    
    float3 normalSampled2 = (NormalMap2.Sample(Sampler, normalMapUV2).rgb * 2.0f) - 1.0f;
    normal += normalize(mul(normalSampled2, tbn));
    
    normal = normalize(normal);

    float3 l = normalize(DirLightDirection) * -1.0f;
    float3 view = normalize(CameraPosition - Input.posWS.xyz);
    float3 h = normalize(l + view);

    float roughness = 0.08f;
    float reflectance = 0.55f;
    
    float ndotl = dot(normal, l);
    float3 f0 = 0.16f * reflectance * reflectance;

    float normalDistribution = DistributionGGX(normal, h, roughness);
    float3 fresnelReflectance = FresnelShlick(f0, view, h);
    float geometryTerm = GeometrySmith(roughness, normal, view, l);

    float3 specularFactor = (geometryTerm * normalDistribution) * fresnelReflectance * 125.0f * ndotl;

    float3 diffuseColor = WaterColor.xyz * ndotl;

    float t = 1.0f - pow(saturate(dot(n, view)), 2.0f); // Using n over normal here bc I think it looks better
    t = remap(t, 0.0f, 1.0f, 0.25f, 0.9f);

    float3 finalColor = diffuseColor + specularFactor;
    
    return float4(finalColor, t);
}