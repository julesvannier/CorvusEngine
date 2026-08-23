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
    float SSRIntensity;
    float EnviroIntensity;
    float DistortionFactor;
    float SpecularIntensity;
    float SpecularNoiseTilingFactor;
    float Padding3;
    float4 SSRSettings;
}

SamplerState Sampler : register(s3);
Texture2D NormalMap : register(t4);
Texture2D NormalMap2 : register(t5);
Texture2D Depth : register(t6);
Texture2D Albedo : register(t7);
Texture2D Normal : register(t8);
TextureCube CubeMap : register(t9);
Texture2D NoiseTex : register(t10);

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

bool ValidScreenUV(float2 uv)
{
    return uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0;
}

static float EPSILON = 0.000001f;

float4 Main(PixelIn Input) : SV_Target
{
    float3 n = normalize(Input.normal);
    float3x3 tbn = float3x3(normalize(Input.tangent), normalize(Input.binormal), n);

    // Scrolling normals
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

    // Specular
    float3 f0 = 0.16f * Reflectance * Reflectance;
    
    float normalDistribution = DistributionGGX(normal, h, Roughness);
    float3 fresnelReflectance = FresnelShlick(f0, view, h);
    float geometryTerm = GeometrySmith(Roughness, normal, view, l);

    float specularNoise = NoiseTex.Sample(Sampler, normalMapUV * SpecularNoiseTilingFactor).r;
    specularNoise *= NoiseTex.Sample(Sampler, normalMapUV2 * SpecularNoiseTilingFactor).r; 
    specularNoise *= NoiseTex.Sample(Sampler, Input.uv * SpecularNoiseTilingFactor).r; 
    
    float3 specularFactor = (geometryTerm * normalDistribution) * fresnelReflectance * SpecularIntensity * ndotl * specularNoise;

    // Enviro reflections
    float3 r  = normalize(reflect(-view, normal));
    float3 cubeMapColor = CubeMap.Sample(Sampler, r).xyz * EnviroIntensity;

    // SSR
    float3 normalSSR = normal;

    float3 normalVS = normalize(mul(float4(normalSSR, 0.0f), View).xyz);
    float3 viewDirVS = -normalize(Input.posView.xyz);
    float3 reflectionVS = normalize(reflect(-viewDirVS, normalVS));

    float forwardStepsMax = SSRSettings.x;
    float backwardStepsMax = SSRSettings.y;
    float stepCount = 1.0f; // avoid self collision
    float forwardStepsCount = forwardStepsMax;
    float forwardStepLength = SSRSettings.z;
    float3 rayMarchPosition = Input.posView.xyz;
    float4 rayMarchTexPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 viewSpacePosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float sceneZ = 0.0f;
    bool rayHit = false;
    float depthTolerance = 0.001f;

    while (stepCount < forwardStepsMax)
    {
        rayMarchPosition += reflectionVS * forwardStepLength;
        rayMarchTexPosition = mul(float4(rayMarchPosition, 1.0f), Proj); // View to NDC

        if (rayMarchTexPosition.w < EPSILON)
            rayMarchTexPosition.w = EPSILON;
        
        rayMarchTexPosition.xy /= rayMarchTexPosition.w;
        rayMarchTexPosition.xy = float2(rayMarchTexPosition.x, -rayMarchTexPosition.y) * 0.5 + 0.5; // NDC to TexCoords

        if (!ValidScreenUV(rayMarchTexPosition.xy))
        {
            rayHit = false;
            break;
        }
        
        float sceneDepth = Depth.Sample(Sampler, rayMarchTexPosition.xy).r;

        float4 clipSpacePosition = float4(rayMarchTexPosition.xy * 2.0 - 1.0, sceneDepth, 1.0);
        clipSpacePosition.y *= -1.0;
        
        float4 worldSpacePosition = mul(clipSpacePosition, InvViewProj); // NDC to World
        worldSpacePosition /= worldSpacePosition.w;
        
        viewSpacePosition = mul(worldSpacePosition, View); // World back to View

        sceneZ = viewSpacePosition.z;

        if (sceneZ <= rayMarchPosition.z + depthTolerance)
        {
            forwardStepsCount = stepCount;
            stepCount = forwardStepsMax;
            rayHit = true;
        }
        else
        {
            stepCount++;
        }
    }

    if (forwardStepsCount < forwardStepsMax && rayHit)
    {
        rayHit = false;
        stepCount = 0.0f;
        while (stepCount < backwardStepsMax)
        {
            rayMarchPosition -= reflectionVS * forwardStepLength / backwardStepsMax;
            rayMarchTexPosition = mul(float4(rayMarchPosition, 1.0f), Proj); // View to NDC

            if (rayMarchTexPosition.w < EPSILON)
                rayMarchTexPosition.w = EPSILON;
        
            rayMarchTexPosition.xy /= rayMarchTexPosition.w;
            rayMarchTexPosition.xy = float2(rayMarchTexPosition.x, -rayMarchTexPosition.y) * 0.5 + 0.5; // NDC to TexCoords

            if (!ValidScreenUV(rayMarchTexPosition.xy))
            {
                rayHit = false;
                break;
            }
        
            float sceneDepth = Depth.Sample(Sampler, rayMarchTexPosition.xy).r;

            float4 clipSpacePosition = float4(rayMarchTexPosition.xy * 2.0 - 1.0, sceneDepth, 1.0);
            clipSpacePosition.y *= -1.0;
        
            float4 worldSpacePosition = mul(clipSpacePosition, InvViewProj); // NDC to World
            worldSpacePosition /= worldSpacePosition.w;
        
            viewSpacePosition = mul(worldSpacePosition, View); // World back to View

            sceneZ = viewSpacePosition.z;

            if (sceneZ > rayMarchPosition.z - depthTolerance)
            {
                stepCount = backwardStepsMax;
                rayHit = true;
            }
            else
            {
                stepCount++;
            }
        }
    }

    float3 ssrColor = float3(0.0f, 0.0f, 0.0f);
    float ssrFactor = 0.0f;
    
    if (rayHit && ValidScreenUV(rayMarchTexPosition.xy))
    {
        float3 ssrNormal = normalize(Normal.Sample(Sampler, rayMarchTexPosition.xy).xyz);
        ssrNormal = normalize(mul(float4(ssrNormal, 0.0f), View).xyz);
        ssrColor = Albedo.Sample(Sampler, rayMarchTexPosition.xy).xyz;

        if (ssrColor.x > 0.01f && ssrColor.y > 0.01f && ssrColor.z > 0.01f)
        {
            float2 edgeFade = smoothstep(0.0f, 0.1f, rayMarchTexPosition.xy) * smoothstep(1.0f, 0.9f, rayMarchTexPosition.xy);
            float edgeFadeAmount = edgeFade.x * edgeFade.y;
    
            ssrFactor = (1.0f - saturate(dot(n, view)))
                        * edgeFadeAmount
                        * (1.0f - saturate(dot(ssrNormal, normalVS)))
                        * (1.0f / (1.0f + abs(sceneZ - rayMarchPosition.z) * SSRSettings.w))
                        * (1.0f - forwardStepsCount / forwardStepsMax);
        }
    }

    if (SSRIntensity <= 0.0f)
        ssrFactor = 0.0f;

    ssrColor = ssrColor * SSRIntensity;

    // Refraction distortion
    float4 texCoords = mul(float4(Input.posView.xyz, 1.0f), Proj);
    texCoords.xy /= texCoords.w;
    texCoords.xy = float2(texCoords.x, -texCoords.y) * 0.5f + 0.5f;

    float2 distortedTexCoords = texCoords.xy + (((normal.xz + normal.xy) * 0.5f) * DistortionFactor);
    float distortedDepth = Depth.Sample(Sampler, distortedTexCoords).r;
    
    float4 clipSpacePosition = float4(distortedTexCoords.xy * 2.0 - 1.0, distortedDepth, 1.0);
    clipSpacePosition.y *= -1.0;
    float4 worldSpacePosition = mul(clipSpacePosition, InvViewProj);
    worldSpacePosition /= worldSpacePosition.w;
    
    float2 refractionTexCoords = (worldSpacePosition.y < Input.posWS.y + depthTolerance) ? distortedTexCoords : texCoords.xy;

    float3 refractionColor = Albedo.Sample(Sampler, refractionTexCoords.xy).xyz;

    // Opacity
    float t = 1.0f - pow(saturate(dot(normal, view)), 2.0f); // Using n over normal here is also possible
    t = remap(t, 0.0f, 1.0f, 0.55f, 0.95f);

    // Diffuse
    float ndotup = saturate(dot(normal, float3(0.0f, 1.0f, 0.0f)));
    ndotup = saturate(pow(ndotup, 5.0f));
    
    float3 waterColor = lerp(WaterColor2.xyz, WaterColor.xyz, ndotup);

    float3 color = lerp(waterColor, ssrColor, ssrFactor);

    color = lerp(refractionColor, color, t);

    float3 diffuseColor = color * ndotl;

    float3 finalColor = (diffuseColor + specularFactor + cubeMapColor) * DirLightIntensity;
    
    return float4(finalColor, 1.0f /* transparency faked in distortions. Ugly with distortion + real obj under it */);
}