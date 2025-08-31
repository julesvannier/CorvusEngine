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
    float4 WaterColor2;
    float WavesScalar;
    float Roughness;
    float Reflectance;
    float NormalScrollSpeed;
    float NormalTilingFactor;
    float NormalTilingFactor2;
    float2 Padding3;
    row_major float4x4 View;
    row_major float4x4 Proj;
}

SamplerState Sampler : register(s3);
Texture2D NormalMap : register(t4);
Texture2D NormalMap2 : register(t5);
// Texture2D Depth : register(t6);

struct PixelIn
{
    float4 pos : SV_POSITION;
    float4 posWS : TEXCOORD0;
    float4 posView : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
};

float remap(float value, float inMin, float inMax, float outMin, float outMax)
{
    return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
}

static float EPSILON = 0.000001f;

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
    
    float ndotl = dot(normal, l);
    float3 f0 = 0.16f * Reflectance * Reflectance;
    
    float normalDistribution = DistributionGGX(normal, h, Roughness);
    float3 fresnelReflectance = FresnelShlick(f0, view, h);
    float geometryTerm = GeometrySmith(Roughness, normal, view, l);
    
    float3 specularFactor = (geometryTerm * normalDistribution) * fresnelReflectance * 125.0f * ndotl;

    // view -> ndc -> texcoords
    // float4 posNDC = mul(float4(Input.posView.xyz, 1.0f), Proj);
    // if (posNDC.w < EPSILON)
    //     posNDC.w = EPSILON;
    //
    // posNDC.xy /= posNDC.w;
    // posNDC.xy = float2(posNDC.x, -posNDC.y) * 0.5 + 0.5;
    //
    // float sceneDepth = Depth.Sample(Sampler, posNDC.xy).r;

    // view space pos
    // float4 clipSpacePosition = float4(posNDC.xy * 2.0 - 1.0, sceneDepth, 1.0);
    // clipSpacePosition.y *= -1.0;
    //
    // float4 worldSpacePosition = mul(clipSpacePosition, InvViewProj);
    // worldSpacePosition /= worldSpacePosition.w;
    //
    // float4 viewSpacePosition = mul(worldSpacePosition, View);

    float ndotup = saturate(dot(normal, float3(0.0f, 1.0f, 0.0f)));
    ndotup = saturate(pow(ndotup, 5.0f));
    
    float3 waterColor = lerp(WaterColor2.xyz, WaterColor.xyz, ndotup);
    float3 diffuseColor = waterColor * ndotl;
    
    float t = 1.0f - pow(saturate(dot(n, view)), 2.0f); // Using n over normal here bc I think it looks better
    t = remap(t, 0.0f, 1.0f, 0.25f, 0.9f);
    
    float3 finalColor = (diffuseColor + specularFactor) * DirLightIntensity;
    
    return float4(finalColor, t);
}