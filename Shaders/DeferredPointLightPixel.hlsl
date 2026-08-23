#include "Shaders/PBR.hlsl"

Texture2D Albedo : register(t2);
Texture2D Normal : register(t3);
Texture2D MetallicRoughness : register(t4);
Texture2D Depth : register(t5);

cbuffer CBuf : register(b0)
{
    row_major float4x4 View;
    row_major float4x4 Proj;
    float Time;
    float3 CameraPosition;
    int Mode;
    float3 DirLightDirection;
    float DirLightIntensity;
    float2 ScreenDimensions;
    float Padding;
    row_major float4x4 InvViewProj;
};

struct PointLight
{
    float3 Position;
    float Radius;
    // 16 bytes boundary 
    float4 Color;
    // 16 bytes boundary 
    float ConstantAttenuation;
    float LinearAttenuation;
    float QuadraticAttenuation;
    float Padding3;
};

StructuredBuffer<PointLight> InstancesData2 : register(t6, space2);

struct PixelIn
{
    float4 Position : SV_POSITION;
    float3 CameraPosition : TEXCOORD0;
    float3 ObjectPosition : TEXCOORD1;
    int InstanceIdx : TEXCOORD2;
};

float DoAttenuation(PointLight light, float distance)
{
    return 1.0f / (light.ConstantAttenuation + light.LinearAttenuation * distance + light.QuadraticAttenuation * distance * distance);
}

float4 Main(PixelIn Input) : SV_TARGET
{
    PointLight lightInfo = InstancesData2[Input.InstanceIdx];
    float4 albedo = float4(Albedo.Load(int3(Input.Position.xy, 0)).xyz, 1.0);
    float3 normal = normalize(Normal.Load(int3(Input.Position.xy, 0)).xyz);
    float3 metallicRoughness = MetallicRoughness.Load(int3(Input.Position.xy, 0)).xyz;
    float depth = Depth.Load(int3(Input.Position.xy, 0)).x;

    float2 Texcoord = Input.Position.xy / ScreenDimensions;
    float4 clipSpacePosition = float4(Texcoord * 2.0 - 1.0, depth, 1.0);
    clipSpacePosition.y *= -1.0;

    float4 worldSpacePosition = mul(clipSpacePosition, InvViewProj);
    worldSpacePosition /= worldSpacePosition.w;

    float3 view = normalize(Input.CameraPosition - worldSpacePosition.xyz);

    float3 l = Input.ObjectPosition - worldSpacePosition.xyz;
    float d = length(l);
    if(d > lightInfo.Radius)
        discard;

    float3 lightVector = normalize(l);
    float attenuation = DoAttenuation(lightInfo, d);
    float3 radiance = lightInfo.Color.xyz * attenuation;
    float3 F0 = lerp(Fdielectric, albedo.xyz, metallicRoughness.b);

    float3 finalLight = PBR(F0, normal, view, lightVector, normalize(lightVector + view), radiance, albedo.xyz, metallicRoughness.g, metallicRoughness.b);

    return float4(finalLight, 1.0);
}