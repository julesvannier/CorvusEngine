#include "Shaders/PBR.hlsl"

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
    bool AOEnabled;
    float2 Padding2;
};

SamplerState Sampler : register(s3);
Texture2D Albedo : register(t4);
Texture2D Normal : register(t5);
Texture2D MetallicRoughness : register(t6);
TextureCube Irradiance : register(t7);
TextureCube PrefilterEnvMap : register(t8);
// Texture2D BRDFLut : register(t8);

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

    float roughness = metallicRoughness.g;
    float metallic = metallicRoughness.b;

    float4 lightColor = float4(1.0, 1.0, 1.0, 1.0) * DirLightIntensity;
    float3 dirLightVec = normalize(DirLightDirection) * -1.0f; // l (norm vec pointing toward light direction)

    float3 view = normalize(CameraPosition - Input.PositionWS.xyz);
    float3 F0 = lerp(Fdielectric, albedo.xyz, metallic);

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

    float3 outLight = (ambiantLight * DirLightIntensity) + finalLight;

    return float4(outLight, OpacityData[Input.instanceID]);
}